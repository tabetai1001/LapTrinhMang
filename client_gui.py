import tkinter as tk
from tkinter import messagebox, ttk
from ctypes import *
import json
import time

# --- THEME COLORS ---
BG_GRADIENT_START = "#1a1a2e"
BG_GRADIENT_END = "#16213e"
PRIMARY_COLOR = "#0f3460"
SECONDARY_COLOR = "#533483"
ACCENT_COLOR = "#e94560"
SUCCESS_COLOR = "#06d6a0"
WARNING_COLOR = "#ffd166"
DANGER_COLOR = "#ef476f"
TEXT_LIGHT = "#ffffff"
TEXT_DARK = "#2b2d42"
CARD_BG = "#edf2f4"

# --- 1. SETUP DLL ---
try:
    lib = CDLL("./client_network.dll")
    lib.connect_to_server.argtypes = [c_char_p, c_int]
    lib.connect_to_server.restype = c_int
    lib.send_request_and_wait.argtypes = [c_char_p]
    lib.send_request_and_wait.restype = c_char_p
except:
    messagebox.showerror("Lỗi", "Không tìm thấy client_network.dll")
    exit()

current_user = ""
is_in_game = False
last_player_list = []  # Lưu danh sách người chơi lần trước
list_players_items = []  # Lưu các widget items trong danh sách người chơi
selected_player_index = -1  # Index của người chơi được chọn
list_players_container = None  # Container chứa danh sách người chơi
last_lobby_refresh = 0  # Thời gian lần cuối refresh lobby

# --- 2. LOGIC FUNCTIONS ---
def send_json(data):
    json_str = json.dumps(data)
    res_ptr = lib.send_request_and_wait(json_str.encode('utf-8'))
    try:
        return json.loads(res_ptr.decode('utf-8'))
    except:
        return {}

def btn_connect_click():
    ip = entry_ip.get()
    try: port = int(entry_port.get())
    except: 
        messagebox.showerror("Loi", "Port khong hop le!")
        return
    if lib.connect_to_server(ip.encode('utf-8'), port):
        frame_connect.pack_forget()
        frame_login.pack(fill=tk.BOTH, expand=True)
    else: 
        messagebox.showerror("Loi ket noi", "Khong the ket noi den server!\nVui long kiem tra IP va Port.")

def btn_login_click():
    global current_user
    user = entry_user.get(); pwd = entry_pass.get()
    
    if not user or not pwd:
        messagebox.showwarning("Chu y", "Vui long nhap day du thong tin!")
        return
    
    res = send_json({"type": "LOGIN", "user": user, "pass": pwd})
    if res.get("type") == "LOGIN_SUCCESS":
        current_user = res.get("user")
        lbl_welcome.config(text=f"Xin chao, {current_user}!")
        frame_login.pack_forget()
        frame_lobby.pack(fill=tk.BOTH, expand=True)
        # Refresh lobby ngay sau khi login để có danh sách đầy đủ
        refresh_lobby()
        # BẮT ĐẦU VÒNG LẶP POLLING (Hỏi vòng)
        # Poll ngay lập tức để bắt các thay đổi real-time
        root.after(100, poll_server) 
    else: 
        messagebox.showerror("Dang nhap that bai", res.get("message", "Sai ten dang nhap hoac mat khau"))

def btn_register_click():
    user = entry_user.get(); pwd = entry_pass.get()
    if not user or not pwd:
        messagebox.showwarning("Chu y", "Vui long nhap day du thong tin!")
        return
    
    if len(pwd) < 4:
        messagebox.showwarning("Mat khau yeu", "Mat khau phai co it nhat 4 ky tu!")
        return
    
    res = send_json({"type": "REGISTER", "user": user, "pass": pwd})
    if res.get("type") == "REGISTER_SUCCESS":
        messagebox.showinfo("Thanh cong", f"Dang ky thanh cong!\n{res.get('message', '')}")
        entry_user.delete(0, tk.END)
        entry_pass.delete(0, tk.END)
    else:
        messagebox.showerror("Dang ky that bai", res.get("message", "Ten dang nhap da ton tai"))

def handle_lobby_list_update(res):
    """Xử lý cập nhật lobby list từ server response"""
    global last_player_list, list_players_items, selected_player_index
    
    # Server trả về danh sách players có thể là:
    # - List string đơn giản: ["user1", "user2"]
    # - List dict với status: [{"name": "user1", "status": "FREE"}, ...]
    players_data = res.get("players", [])
    print(f"[Client] handle_lobby_list_update: Received {len(players_data)} players from server")
    
    # Debug: In ra từng player
    for p in players_data:
        if isinstance(p, dict):
            print(f"  - {p.get('name')}: {p.get('status')}")
        else:
            print(f"  - {p} (string format)")
    
    # Chuyển đổi sang format thống nhất
    current_players_display = []
    
    for p in players_data:
        if isinstance(p, dict):
            name = p.get("name", p.get("user", ""))
            status = p.get("status", "FREE")
            
            # Thêm icon và màu trạng thái
            if status == "FREE":
                status_icon = "[FREE]"
                status_text = "Rảnh"
                bg_color = "#c8e6c9"  # Xanh lá nhạt
                fg_color = "#1b5e20"  # Xanh lá đậm
            elif status == "IN_GAME":
                status_icon = ""
                status_text = "Đang chơi"
                bg_color = "#ffcdd2"  # Đỏ nhạt
                fg_color = "#c62828"  # Đỏ đậm
            elif status == "OFFLINE":
                status_icon = ""
                status_text = "Offline"
                bg_color = "#e0e0e0"  # Xám nhạt
                fg_color = "#616161"  # Xám đậm
            else:
                status_icon = "[?]"
                status_text = status
                bg_color = "#fff9c4"  # Vàng nhạt
                fg_color = "#f57f17"  # Vàng đậm
            
            display_text = f"{status_icon} {name} - {status_text}"
            current_players_display.append({
                "text": display_text,
                "name": name,
                "status": status,
                "bg": bg_color,
                "fg": fg_color
            })
        else:
            # Format cũ, chỉ có tên
            display_text = f"[FREE] {p} - Rảnh"
            current_players_display.append({
                "text": display_text,
                "name": p,
                "status": "FREE",
                "bg": "#c8e6c9",
                "fg": "#1b5e20"
            })
    
    # Kiểm tra xem container có tồn tại không
    if list_players_container is None:
        print("[Client]  list_players_container is None, cannot update UI")
        return
    
    # Luôn cập nhật UI (không so sánh) để đảm bảo hiển thị đúng
    current_text_list = [p["text"] for p in current_players_display]
    
    print(f"[Client] Updating UI: old={len(last_player_list)} items, new={len(current_text_list)} items")
    
    # Xóa các items cũ
    for item in list_players_items:
        try:
            item["frame"].destroy()
        except Exception as e:
            print(f"[Client] Error destroying item: {e}")
    list_players_items.clear()
    
    # Tạo các items mới với màu sắc
    for idx, player_info in enumerate(current_players_display):
        item_frame = tk.Frame(list_players_container, 
                             bg=player_info["bg"],
                             relief=tk.FLAT,
                             bd=1,
                             highlightthickness=1,
                             highlightbackground="#ddd")
        item_frame.pack(fill=tk.X, padx=5, pady=2)
        
        item_label = tk.Label(item_frame,
                             text=player_info["text"],
                             font=("Segoe UI", 11),
                             bg=player_info["bg"],
                             fg=player_info["fg"],
                             anchor="w",
                             padx=10,
                             pady=8,
                             cursor="hand2")
        item_label.pack(fill=tk.BOTH, expand=True)
        
        # Bind click event
        def make_click_handler(index, frame, label, orig_bg):
            def on_click(event):
                global selected_player_index
                # Reset tất cả items về màu gốc
                for i, item_data in enumerate(list_players_items):
                    item_data["frame"].config(highlightthickness=1, highlightbackground="#ddd")
                # Highlight item được chọn
                frame.config(highlightthickness=2, highlightbackground=SECONDARY_COLOR)
                selected_player_index = index
            return on_click
        
        click_handler = make_click_handler(idx, item_frame, item_label, player_info["bg"])
        item_label.bind("<Button-1>", click_handler)
        item_frame.bind("<Button-1>", click_handler)
        
        # Hover effect
        def make_hover_handlers(label, frame, bg_color):
            def on_enter(e):
                label.config(font=("Segoe UI", 11, "bold"))
            def on_leave(e):
                label.config(font=("Segoe UI", 11))
            return on_enter, on_leave
        
        on_enter, on_leave = make_hover_handlers(item_label, item_frame, player_info["bg"])
        item_label.bind("<Enter>", on_enter)
        item_label.bind("<Leave>", on_leave)
        
        # Bind scroll chuột cho item mới
        try:
            item_frame.bind("<MouseWheel>", lambda e: list_canvas.yview_scroll(int(-1*(e.delta/120)), "units"))
            item_label.bind("<MouseWheel>", lambda e: list_canvas.yview_scroll(int(-1*(e.delta/120)), "units"))
        except:
            pass
        
        list_players_items.append({
            "frame": item_frame,
            "label": item_label,
            "data": player_info
        })
    
    # Cập nhật thống kê
    total = len(current_players_display)
    free_count = sum(1 for p in current_players_display if p["status"] == "FREE")
    in_game_count = sum(1 for p in current_players_display if p["status"] == "IN_GAME")
    offline_count = sum(1 for p in current_players_display if p["status"] == "OFFLINE")
    
    print(f"[Client] Display stats - Total: {total}, Free: {free_count}, In-Game: {in_game_count}, Offline: {offline_count}")
    
    # Cập nhật status label (nếu tồn tại)
    try:
        status_label.config(text=f" Tổng: {total}  |  🟢 Rảnh: {free_count}  |   Chơi: {in_game_count}  |   Offline: {offline_count}")
    except:
        pass  # status_label chưa được tạo
    
    # Cập nhật last_player_list
    last_player_list = current_text_list.copy()
    
    print(f"[Client]  UI updated successfully!")

def refresh_lobby():
    """Yêu cầu server gửi danh sách lobby và cập nhật UI"""
    print("[Client] Requesting ALL players (include_offline=True)")
    res = send_json({"type": "GET_LOBBY_LIST", "include_offline": True})
    handle_lobby_list_update(res)

def btn_logout_click():
    """Xử lý đăng xuất"""
    global current_user, is_in_game
    
    if is_in_game:
        messagebox.showwarning("Chu y", "Ban dang trong tran dau, khong the dang xuat!")
        return
    
    result = messagebox.askyesno("Xac nhan", "Ban co chac chan muon dang xuat?")
    if result:
        print(f"[Client] User {current_user} logging out")
        
        # Gửi thông báo logout cho server
        res = send_json({"type": "LOGOUT"})
        
        if res.get("type") == "LOGOUT_SUCCESS":
            print("[Client] Logout successful")
            current_user = ""
            is_in_game = False
            
            # Quay về màn hình đăng nhập
            frame_lobby.pack_forget()
            frame_login.pack(fill=tk.BOTH, expand=True)
            
            # Xóa thông tin đăng nhập
            entry_user.delete(0, tk.END)
            entry_pass.delete(0, tk.END)
        else:
            messagebox.showerror("Loi", res.get("message", "Khong the dang xuat"))

# --- CHỨC NĂNG MỚI: MỜI THÁCH ĐẤU ---
def btn_invite_click():
    global selected_player_index
    
    if selected_player_index < 0 or selected_player_index >= len(list_players_items):
        messagebox.showwarning(" Chú ý", "Hãy chọn một người chơi để thách đấu!")
        return
    
    # Lấy thông tin người chơi được chọn
    selected_item = list_players_items[selected_player_index]
    player_data = selected_item["data"]
    target = player_data["name"]
    status = player_data["status"]
    
    if target == current_user:
        messagebox.showwarning(" Chú ý", "Không thể thách đấu chính mình!")
        return
    
    # Kiểm tra trạng thái người chơi
    if status == "IN_GAME":
        messagebox.showwarning(" Chú ý", f"{target} đang trong trận, vui lòng chọn người khác!")
        return
    
    if status == "OFFLINE":
        messagebox.showwarning(" Chú ý", f"{target} đã offline!")
        return
    
    # Popup chọn số câu hỏi với style đẹp
    choice_window = tk.Toplevel(root)
    choice_window.title(" Thách Đấu")
    choice_window.geometry("420x380")
    choice_window.configure(bg=PRIMARY_COLOR)
    choice_window.resizable(False, False)
    
    # Title
    tk.Label(choice_window, text=f" Thách đấu với {target}",
            font=("Segoe UI", 16, "bold"),
            fg=TEXT_LIGHT,
            bg=PRIMARY_COLOR).pack(pady=15)
    
    # Card chứa options
    options_card = create_styled_frame(choice_window, CARD_BG)
    options_card.pack(pady=10, padx=40, fill=tk.X)
    
    tk.Label(options_card, text="Chọn số câu hỏi:",
            font=("Segoe UI", 12, "bold"),
            bg=CARD_BG,
            fg=TEXT_DARK).pack(pady=10)
    
    num_q_var = tk.IntVar(value=5)
    
    # Style cho radiobuttons
    radio_frame = tk.Frame(options_card, bg=CARD_BG)
    radio_frame.pack(pady=5, padx=10)
    
    tk.Radiobutton(radio_frame, text=" 5 câu (Nhanh)",
                  variable=num_q_var, value=5,
                  font=("Segoe UI", 11),
                  bg=CARD_BG, fg=TEXT_DARK,
                  selectcolor=SUCCESS_COLOR,
                  activebackground=CARD_BG).pack(anchor="w", pady=5, padx=20)
    tk.Radiobutton(radio_frame, text=" 10 câu (Trung bình)",
                  variable=num_q_var, value=10,
                  font=("Segoe UI", 11),
                  bg=CARD_BG, fg=TEXT_DARK,
                  selectcolor=WARNING_COLOR,
                  activebackground=CARD_BG).pack(anchor="w", pady=5, padx=20)
    tk.Radiobutton(radio_frame, text=" 15 câu (Dài)",
                  variable=num_q_var, value=15,
                  font=("Segoe UI", 11),
                  bg=CARD_BG, fg=TEXT_DARK,
                  selectcolor=DANGER_COLOR,
                  activebackground=CARD_BG).pack(anchor="w", pady=5, padx=20)
    
    # Thêm padding dưới cho options_card
    tk.Label(options_card, text="", bg=CARD_BG).pack(pady=5)
    
    def send_invite():
        num_questions = num_q_var.get()
        choice_window.destroy()
        
        # Gửi lời mời
        res = send_json({"type": "INVITE_PLAYER", "target": target, "num_questions": num_questions})
        if res.get("type") == "INVITE_SENT_SUCCESS":
            messagebox.showinfo(" Đã gửi", f"Đang chờ {target} trả lời ({num_questions} câu)...")
        else:
            messagebox.showerror(" Lỗi", res.get("message"))
    
    # Buttons - Đặt rõ ràng không bị che
    btn_frame = tk.Frame(choice_window, bg=PRIMARY_COLOR)
    btn_frame.pack(pady=15)
    
    btn_send = create_styled_button(btn_frame, " Gửi lời mời", send_invite, SUCCESS_COLOR, width=15)
    btn_send.pack(side=tk.LEFT, padx=5)
    
    btn_cancel = create_styled_button(btn_frame, " Hủy", choice_window.destroy, DANGER_COLOR, width=10)
    btn_cancel.pack(side=tk.LEFT, padx=5)

# --- CHỨC NĂNG MỚI: POLLING LOOP ---
def poll_server():
    global last_lobby_refresh, my_score, opp_score
    
    # Gửi gói tin POLL để hỏi Server có gì mới không
    res = send_json({"type": "POLL"})
    msg_type = res.get("type")
    
    # Debug: In ra message type và số lượng players nếu có
    if msg_type == "LOBBY_LIST":
        players_count = len(res.get("players", []))
        print(f"[Client]  POLL response: LOBBY_LIST with {players_count} players")
    elif msg_type == "NO_EVENT":
        print(f"[Client]  POLL response: NO_EVENT (no changes)")
    else:
        print(f"[Client]  POLL response: {msg_type}")
    
    if msg_type == "RECEIVE_INVITE":
        inviter = res.get("from")
        num_q = res.get("num_questions", 5)
        # Hiện Popup hỏi ý kiến
        difficulty = " Nhanh" if num_q == 5 else " Trung bình" if num_q == 10 else " Dài"
        ans = messagebox.askyesno(" Thách đấu!", 
                                  f" Người chơi {inviter} muốn thách đấu bạn!\n\n"
                                  f" Số câu hỏi: {num_q} câu ({difficulty})\n\n"
                                  f"Bạn có chấp nhận không?")
        if ans:
            # Đồng ý
            accept_res = send_json({"type": "ACCEPT_INVITE", "from": inviter})
            if not is_in_game:  # Kiểm tra lại trước khi start
                game_key = int(accept_res.get("game_key", 0))
                start_game(inviter, game_key)
        else:
            # Từ chối
            send_json({"type": "REJECT_INVITE", "from": inviter})
            messagebox.showinfo(" Thông báo", f"Đã từ chối lời mời từ {inviter}")
            
    elif msg_type == "GAME_START":
        opponent = res.get("opponent")
        game_key = int(res.get("game_key", 0))
        if not is_in_game:  # Kiểm tra lại trước khi start để tránh start 2 lần
            start_game(opponent, game_key)
    
    elif msg_type == "OPPONENT_QUIT":
        # Đối thủ đã bỏ cuộc
        if is_in_game:
            opponent_name = res.get("opponent", current_opponent)
            print(f"[Client] Opponent {opponent_name} quit the game!")
            # Hiển thị màn hình kết quả với thông báo đối thủ bỏ cuộc
            root.after(100, lambda: show_opponent_quit_result(opponent_name))
    
    elif msg_type == "LOBBY_LIST":
        # Server gửi lobby list khi có thay đổi (có người login/logout)
        if "players" in res:
            try:
                # Luôn cập nhật lobby list khi nhận được, ngay cả khi đang in game
                # (vì list này hiển thị ở lobby screen, không ảnh hưởng game screen)
                handle_lobby_list_update(res)
                print(f"[Client]  Lobby updated successfully")
            except Exception as e:
                print(f"[Client]  Error updating lobby: {e}")
    
    elif msg_type == "NO_EVENT":
        # Không có gì mới, không làm gì cả
        pass
        
    # Lặp lại sau 1 giây
    root.after(1000, poll_server)

# --- CHỨC NĂNG MỚI: MÀN HÌNH GAME ---
current_opponent = ""
current_game_key = 0  # KEY duy nhất của trận đấu hiện tại
current_question_id = 0
question_start_time = 0
timer_seconds = 0
timer_running = False  # Biến kiểm soát timer
my_score = 0
opp_score = 0
total_questions = 5
game_history = []  # Lưu lịch sử câu hỏi và đáp án

def start_game(opponent_name, game_key=0):
    global is_in_game, current_opponent, current_game_key, my_score, opp_score, game_history, total_questions
    is_in_game = True
    current_opponent = opponent_name
    current_game_key = game_key  # Lưu game_key để phân biệt các trận
    my_score = 0
    opp_score = 0
    game_history = []
    
    print(f"[Client] Starting game with {opponent_name}, GAME_KEY={game_key}")
    
    frame_lobby.pack_forget()
    
    # Tạo lại giao diện game nếu các widget đã bị destroy
    setup_game_widgets()
    
    frame_game.pack()
    lbl_opponent.config(text=f"Đối thủ: {opponent_name}")
    lbl_scores.config(text=f"Bạn: 0 | {opponent_name}: 0")
    
    # Yêu cầu câu hỏi đầu tiên
    request_next_question()

def request_next_question():
    global question_start_time, timer_seconds, timer_running
    
    # Reset màu các nút về mặc định
    btn_a.config(bg="SystemButtonFace", fg="black", state=tk.NORMAL)
    btn_b.config(bg="SystemButtonFace", fg="black", state=tk.NORMAL)
    btn_c.config(bg="SystemButtonFace", fg="black", state=tk.NORMAL)
    btn_d.config(bg="SystemButtonFace", fg="black", state=tk.NORMAL)
    lbl_result.config(text="")  # Xóa kết quả câu trước
    
    print(f"[Client] Requesting question...")
    # Gửi yêu cầu lấy câu hỏi
    res = send_json({"type": "REQUEST_QUESTION"})
    
    print(f"[Client] Response: {res}")
    
    if res.get("type") == "QUESTION":
        display_question(res)
        question_start_time = time.time()
        timer_seconds = res.get("max_time", 15)
        timer_running = True  # Bật timer
        update_timer()
    elif res.get("type") == "NO_MORE_QUESTIONS":
        messagebox.showinfo(" Kết thúc", "Đã hết câu hỏi! Đang tính điểm...")
        quit_game()
    elif res.get("type") == "ERROR":
        messagebox.showerror(" Lỗi", f"Lỗi: {res.get('message', 'Unknown error')}")
        quit_game()
    else:
        messagebox.showerror(" Lỗi", f"Phản hồi không xác định: {res.get('type')}")
        quit_game()

def display_question(data):
    global current_question_id, total_questions
    
    current_question_id = data.get("question_id")
    q_num = data.get("question_number")
    total = data.get("total_questions")
    total_questions = total
    question_text = data.get("question")
    options = data.get("options", [])
    
    print(f"[Client] Question {q_num}: {question_text}")
    print(f"[Client] Options: {options}")
    
    # Lưu thông tin câu hỏi vào lịch sử
    game_history.append({
        "question": question_text,
        "options": options.copy(),
        "my_answer": -1,
        "correct_answer": -1,
        "time_taken": 0
    })
    
    lbl_question.config(text=f" Câu {q_num}/{total}:\n{question_text}")
    
    if len(options) >= 4:
        btn_a.config(text=f"A. {options[0]}", state=tk.NORMAL, bg="#3a86ff")
        btn_b.config(text=f"B. {options[1]}", state=tk.NORMAL, bg="#8338ec")
        btn_c.config(text=f"C. {options[2]}", state=tk.NORMAL, bg="#ff006e")
        btn_d.config(text=f"D. {options[3]}", state=tk.NORMAL, bg="#fb5607")
    else:
        messagebox.showerror(" Lỗi", f"Không đủ đáp án! Chỉ có {len(options)} đáp án")

def update_timer():
    global timer_seconds, question_start_time, timer_running
    
    if not is_in_game or not timer_running:
        return
    
    elapsed = time.time() - question_start_time
    remaining = max(0, timer_seconds - int(elapsed))
    
    lbl_timer.config(text=f"Thời gian: {remaining}s")
    
    if remaining > 0:
        root.after(100, update_timer)
    else:
        # Hết giờ, tự động gửi đáp án sai và chuyển câu
        print("[Client] Time's up! Auto-submitting...")
        submit_answer_timeout()

def btn_answer_click(ans_index):
    submit_answer(ans_index)

def submit_answer_timeout():
    """Hết giờ, tự động submit đáp án sai"""
    global game_history, timer_running
    
    # Dừng timer
    timer_running = False
    
    # Đánh dấu tất cả nút là ĐỎ (hết giờ)
    btn_a.config(state=tk.DISABLED, bg="red", fg="white")
    btn_b.config(state=tk.DISABLED, bg="red", fg="white")
    btn_c.config(state=tk.DISABLED, bg="red", fg="white")
    btn_d.config(state=tk.DISABLED, bg="red", fg="white")
    
    lbl_result.config(text="⏰ HẾT GIỜ! Chuyển câu tiếp...", fg="red", font=("Arial", 14, "bold"))
    
    time_taken = time.time() - question_start_time
    
    # Lưu đáp án timeout
    if len(game_history) > 0:
        game_history[-1]["my_answer"] = -1
        game_history[-1]["is_timeout"] = True  # Đánh dấu timeout
    
    # Gửi đáp án -1 (không trả lời)
    res = send_json({
        "type": "SUBMIT_ANSWER",
        "question_id": current_question_id,
        "answer_index": -1,
        "time_taken": time_taken
    })
    
    if res.get("type") == "ANSWER_RESULT":
        process_answer_result(res, -1, time_taken)

def process_answer_result(res, ans_index, time_taken):
    """Xử lý kết quả trả lời chung"""
    global my_score, opp_score, game_history
    
    is_correct = res.get("is_correct")
    correct_ans = res.get("correct_answer")
    earned = res.get("earned_score", 0)
    my_score = res.get("your_total_score", 0)
    opp_score = res.get("opponent_score", 0)
    curr_q = res.get("current_question", 1)
    total_q = res.get("total_questions", 5)
    
    # Cập nhật lịch sử câu hỏi cuối cùng
    if len(game_history) > 0:
        game_history[-1]["correct_answer"] = correct_ans
        game_history[-1]["time_taken"] = time_taken
    
    # Cập nhật điểm ngay lập tức
    lbl_scores.config(text=f"Bạn: {my_score} | {current_opponent}: {opp_score}")
    
    # Đổi màu các nút dựa trên kết quả
    buttons = [btn_a, btn_b, btn_c, btn_d]
    for i, btn in enumerate(buttons):
        if i == correct_ans:
            btn.config(bg=SUCCESS_COLOR, fg=TEXT_LIGHT)
        elif i == ans_index and not is_correct:
            btn.config(bg=DANGER_COLOR, fg=TEXT_LIGHT)
        elif ans_index == -1:  # Timeout: đáp án đúng cũng đỏ
            btn.config(bg=DANGER_COLOR, fg=TEXT_LIGHT)
        else:
            btn.config(bg="#6c757d", fg=TEXT_LIGHT)
    
    # Hiển thị thông báo kết quả trong label
    if ans_index == -1:
        lbl_result.config(text="⏰ HẾT GIỜ!", fg="red", font=("Arial", 14, "bold"))
    elif is_correct:
        lbl_result.config(text=f" ĐÚNG! +{earned} điểm", fg="green", font=("Arial", 14, "bold"))
    else:
        lbl_result.config(text=f" SAI! Đáp án đúng: {chr(65 + correct_ans)}", fg="red", font=("Arial", 14, "bold"))
    
    # Kiểm tra trạng thái game
    game_status = res.get("game_status")
    print(f"[Client] Current Q: {curr_q}/{total_q}, Status: {game_status}")
    
    # Xử lý theo trạng thái
    if game_status == "FINISHED":
        you_win = res.get("you_win", my_score > opp_score)
        print(f"[Client] FINISHED from SUBMIT_ANSWER! you_win={you_win}")
        root.after(2000, lambda: show_game_result(you_win, my_score, opp_score))
    elif game_status == "WAITING_OPPONENT":
        print(f"[Client] Waiting for opponent to finish...")
        lbl_result.config(text="⏳ Đang chờ đối thủ hoàn thành...", fg="blue", font=("Arial", 12, "bold"))
        # Vô hiệu hóa tất cả nút câu hỏi
        btn_a.config(state=tk.DISABLED)
        btn_b.config(state=tk.DISABLED)
        btn_c.config(state=tk.DISABLED)
        btn_d.config(state=tk.DISABLED)
        # Polling để chờ đối thủ xong
        root.after(2000, wait_for_opponent_finish)
    else:
        # Chuyển câu tiếp theo sau 1.5 giây
        root.after(1500, request_next_question)

def submit_answer(ans_index):
    global question_start_time, game_history, timer_running
    
    # Dừng timer ngay lập tức
    timer_running = False
    
    time_taken = time.time() - question_start_time
    
    # Lưu đáp án vào lịch sử
    if len(game_history) > 0:
        game_history[-1]["my_answer"] = ans_index
    
    print(f"[Client] Submitting answer {ans_index}, time: {time_taken:.2f}s")
    
    # Vô hiệu hóa các nút
    btn_a.config(state=tk.DISABLED)
    btn_b.config(state=tk.DISABLED)
    btn_c.config(state=tk.DISABLED)
    btn_d.config(state=tk.DISABLED)
    
    # Gửi đáp án lên server
    res = send_json({
        "type": "SUBMIT_ANSWER",
        "question_id": current_question_id,
        "answer_index": ans_index,
        "time_taken": time_taken
    })
    
    print(f"[Client] Answer response: {res}")
    
    if res.get("type") == "ANSWER_RESULT":
        process_answer_result(res, ans_index, time_taken)

def wait_for_opponent_finish():
    """Chờ đối thủ hoàn thành khi mình đã xong"""
    global is_in_game, my_score, opp_score
    
    if not is_in_game:
        print("[Client] wait_for_opponent_finish: is_in_game=False, stopping poll")
        return
    
    # Gửi request kiểm tra trạng thái game
    res = send_json({"type": "CHECK_GAME_STATUS"})
    
    print(f"[Client] wait_for_opponent_finish: Response type={res.get('type')}, status={res.get('game_status')}")
    
    if res.get("type") == "GAME_STATUS_UPDATE":
        game_status = res.get("game_status")
        opp_score = res.get("opponent_score", opp_score)
        my_score = res.get("your_total_score", my_score)  # Cập nhật cả điểm của mình
        lbl_scores.config(text=f"Bạn: {my_score} | {current_opponent}: {opp_score}")
        
        if game_status == "FINISHED":
            # Cả 2 đã hoàn thành, hiện kết quả
            you_win = res.get("you_win", my_score > opp_score)
            print(f"[Client] Game FINISHED from CHECK_GAME_STATUS! you_win={you_win}, my_score={my_score}, opp_score={opp_score}")
            show_game_result(you_win, my_score, opp_score)
        else:
            # Vẫn đang chờ, poll tiếp
            print(f"[Client] Still waiting... my_score={my_score}, opp_score={opp_score}")
            root.after(2000, wait_for_opponent_finish)
    elif res.get("type") == "ERROR":
        # Có lỗi, có thể game đã kết thúc, thử lại 1 lần nữa
        print(f"[Client] ERROR response: {res.get('message')}, retrying once...")
        root.after(1000, wait_for_opponent_finish)
    else:
        # Không nhận được phản hồi hợp lệ, thử lại
        print(f"[Client] Invalid response type={res.get('type')}, retrying...")
        root.after(2000, wait_for_opponent_finish)

def show_game_result(you_win, final_my_score, final_opp_score):
    global is_in_game, current_game_key
    is_in_game = False
    
    print(f"[Client] Showing result for GAME_KEY={current_game_key}: Win={you_win}, MyScore={final_my_score}, OppScore={final_opp_score}, History={len(game_history)} questions")
    
    # Xóa nội dung frame_game hiện tại
    for widget in frame_game.winfo_children():
        widget.destroy()
    
    frame_game.configure(bg=PRIMARY_COLOR)
    
    # Tiêu đề kết quả với card đẹp
    result_card = create_styled_frame(frame_game, CARD_BG if you_win else "#ffe5e5")
    result_card.pack(pady=20, padx=40, fill=tk.X)
    
    result_label = tk.Label(result_card, 
        text=" CHIẾN THẮNG! " if you_win else " THUA CUỘC ",
        font=("Segoe UI", 28, "bold"),
        fg=SUCCESS_COLOR if you_win else DANGER_COLOR,
        bg=CARD_BG if you_win else "#ffe5e5")
    result_label.pack(pady=20)
    
    # Điểm số
    score_frame = tk.Frame(result_card, bg=CARD_BG if you_win else "#ffe5e5")
    score_frame.pack(pady=10)
    
    tk.Label(score_frame, text=f" Bạn: {final_my_score} điểm",
            font=("Segoe UI", 18, "bold"),
            fg=TEXT_DARK,
            bg=CARD_BG if you_win else "#ffe5e5").pack(pady=3)
    tk.Label(score_frame, text=f" Đối thủ ({current_opponent}): {final_opp_score} điểm",
            font=("Segoe UI", 18, "bold"),
            fg=TEXT_DARK,
            bg=CARD_BG if you_win else "#ffe5e5").pack(pady=3)
    
    # Thời gian trả lời trung bình
    if len(game_history) > 0:
        avg_time = sum(h["time_taken"] for h in game_history) / len(game_history)
        tk.Label(score_frame, text=f"⏱ Thời gian trung bình: {avg_time:.2f}s",
                font=("Segoe UI", 13),
                fg="gray",
                bg=CARD_BG if you_win else "#ffe5e5").pack(pady=10)
    
    # Lịch sử câu hỏi
    tk.Label(frame_game, text=" Chi tiết các câu hỏi:",
            font=("Segoe UI", 16, "bold"),
            fg=TEXT_LIGHT,
            bg=PRIMARY_COLOR).pack(pady=15)
    
    # Frame cuộn để hiển thị lịch sử
    canvas = tk.Canvas(frame_game, height=250, bg=PRIMARY_COLOR, highlightthickness=0)
    scrollbar = tk.Scrollbar(frame_game, orient="vertical", command=canvas.yview)
    scrollable_frame = tk.Frame(canvas, bg=PRIMARY_COLOR)
    
    scrollable_frame.bind(
        "<Configure>",
        lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
    )
    
    canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
    canvas.configure(yscrollcommand=scrollbar.set)
    
    # Bind scroll chuột cho game history
    def on_game_mousewheel(event):
        canvas.yview_scroll(int(-1*(event.delta/120)), "units")
    
    canvas.bind("<MouseWheel>", on_game_mousewheel)
    scrollable_frame.bind("<MouseWheel>", on_game_mousewheel)
    
    # Hiển thị từng câu hỏi
    for i, h in enumerate(game_history):
        q_frame = tk.Frame(scrollable_frame, relief=tk.FLAT, borderwidth=1, bg=CARD_BG)
        q_frame.pack(fill=tk.BOTH, padx=10, pady=5)
        
        # Bind scroll cho frame này
        q_frame.bind("<MouseWheel>", on_game_mousewheel)
        
        q_label = tk.Label(q_frame, text=f" Câu {i+1}: {h['question']}",
                font=("Segoe UI", 11, "bold"),
                wraplength=700,
                bg=CARD_BG,
                fg=TEXT_DARK)
        q_label.pack(anchor="w", padx=10, pady=5)
        q_label.bind("<MouseWheel>", on_game_mousewheel)
        
        for j, opt in enumerate(h['options']):
            color = TEXT_DARK
            prefix = chr(65 + j)
            opt_bg = CARD_BG
            
            # Xử lý timeout: tất cả đáp án đều đỏ
            if h.get('is_timeout', False):
                color = DANGER_COLOR
                if j == h['correct_answer']:
                    prefix += " (Đúng)"
                    opt_bg = "#c8e6c9"
            elif j == h['correct_answer']:
                color = SUCCESS_COLOR
                prefix += " "
                opt_bg = "#c8e6c9"
            elif j == h['my_answer'] and j != h['correct_answer']:
                color = DANGER_COLOR
                prefix += " "
                opt_bg = "#ffcdd2"
            elif j == h['my_answer']:
                prefix += " "
            
            opt_label = tk.Label(q_frame, text=f"{prefix}. {opt}",
                               fg=color,
                               bg=opt_bg,
                               font=("Segoe UI", 10),
                               anchor="w")
            opt_label.pack(anchor="w", padx=20, fill=tk.X)
            opt_label.bind("<MouseWheel>", on_game_mousewheel)
        
        # Hiển thị thời gian với cảnh báo nếu timeout
        time_text = f"⏱ Thời gian: {h['time_taken']:.2f}s"
        if h.get('is_timeout', False):
            time_text += " ⏰ HẾT GIỜ"
        time_label = tk.Label(q_frame, text=time_text,
                font=("Segoe UI", 9),
                fg=DANGER_COLOR if h.get('is_timeout', False) else "gray",
                bg=CARD_BG)
        time_label.pack(anchor="w", padx=20, pady=(5, 10))
        time_label.bind("<MouseWheel>", on_game_mousewheel)
    
    canvas.pack(side="left", fill="both", expand=True)
    scrollbar.pack(side="right", fill="y")
    
    # Nút đóng
    def close_result():
        global game_history, is_in_game, current_opponent, current_game_key
        print(f"[Client] Closing result screen, resetting state. Old GAME_KEY={current_game_key}")
        
        # Clear game history để không bị trộn với ván tiếp theo
        game_history = []
        is_in_game = False  # Reset trước để poll_server hoạt động
        current_opponent = ""  # Reset opponent để tránh nhầm lẫn
        current_game_key = 0  # Reset game_key
        
        # Không cần gửi QUIT_GAME vì đã được reset trong CHECK_GAME_STATUS hoặc SUBMIT_ANSWER
        # send_json({"type": "QUIT_GAME"})  # REMOVED: Đã reset rồi
        
        frame_game.pack_forget()
        frame_lobby.pack()
        
        # Cập nhật ngay danh sách người chơi
        try:
            refresh_lobby()
        except Exception as e:
            print(f"[Client] Error refreshing lobby: {e}")
        
        # KHÔNG gọi root.after(1000, poll_server) vì poll_server đã tự lặp lại
    
    create_styled_button(frame_game, " Về Lobby", close_result, SECONDARY_COLOR, width=20).pack(pady=20)

def show_opponent_quit_result(opponent_name):
    """Hiển thị kết quả khi đối thủ bỏ cuộc"""
    global is_in_game, game_history, my_score, opp_score
    
    is_in_game = False
    print(f"[Client] Opponent {opponent_name} quit. Showing victory screen.")
    
    # Xóa nội dung frame_game hiện tại
    for widget in frame_game.winfo_children():
        widget.destroy()
    
    frame_game.configure(bg=PRIMARY_COLOR)
    
    # Card thông báo đối thủ bỏ cuộc
    result_card = create_styled_frame(frame_game, "#e8f5e9")  # Xanh lá nhạt
    result_card.pack(pady=40, padx=40, fill=tk.BOTH, expand=True)
    
    # Icon và tiêu đề
    tk.Label(result_card,
            text="",
            font=("Segoe UI", 60),
            bg="#e8f5e9").pack(pady=20)
    
    tk.Label(result_card, 
            text="CHIẾN THẮNG!",
            font=("Segoe UI", 32, "bold"),
            fg=SUCCESS_COLOR,
            bg="#e8f5e9").pack(pady=10)
    
    # Thông báo
    tk.Label(result_card,
            text=f"Đối thủ {opponent_name} đã bỏ cuộc!",
            font=("Segoe UI", 18),
            fg=TEXT_DARK,
            bg="#e8f5e9").pack(pady=10)
    
    tk.Label(result_card,
            text=" Bạn giành chiến thắng! ",
            font=("Segoe UI", 16, "bold"),
            fg=SUCCESS_COLOR,
            bg="#e8f5e9").pack(pady=20)
    
    # Điểm số hiện tại
    score_frame = tk.Frame(result_card, bg="#e8f5e9")
    score_frame.pack(pady=15)
    
    tk.Label(score_frame,
            text=f"Điểm của bạn: {my_score}",
            font=("Segoe UI", 14),
            fg=TEXT_DARK,
            bg="#e8f5e9").pack()
    
    # Nút về lobby
    def back_to_lobby():
        global game_history, is_in_game, current_opponent, current_game_key
        game_history = []
        is_in_game = False
        current_opponent = ""
        current_game_key = 0
        
        frame_game.pack_forget()
        frame_lobby.pack(fill=tk.BOTH, expand=True)
        
        try:
            refresh_lobby()
        except Exception as e:
            print(f"[Client] Error refreshing lobby: {e}")
    
    create_styled_button(result_card, " Về Lobby", back_to_lobby, SECONDARY_COLOR, width=20).pack(pady=30)

def quit_game():
    global is_in_game, game_history, current_opponent, current_game_key
    print(f"[Client] Quitting game mid-match. GAME_KEY={current_game_key}")
    
    # Lưu thông tin trước khi reset
    was_in_game = is_in_game
    opponent = current_opponent
    game_key = current_game_key
    
    is_in_game = False  # Reset trước
    game_history = []  # Clear history khi thoát giữa chừng
    current_opponent = ""  # Reset opponent
    current_game_key = 0  # Reset game_key
    
    # Gửi thông báo thoát game để reset trạng thái server và thông báo đối thủ
    if was_in_game and opponent:
        send_json({"type": "QUIT_GAME", "game_key": game_key, "opponent": opponent})
        print(f"[Client] Notified server about quitting game with {opponent}")
    else:
        send_json({"type": "QUIT_GAME"})
    
    frame_game.pack_forget()
    frame_lobby.pack(fill=tk.BOTH, expand=True)
    
    # Cập nhật danh sách người chơi
    try:
        refresh_lobby()
    except Exception as e:
        print(f"[Client] Error refreshing lobby: {e}")
    
    # KHÔNG gọi root.after(1000, poll_server) vì nó đã tự lặp lại

# --- HELPER FUNCTIONS FOR STYLING ---
def create_styled_button(parent, text, command, bg_color=PRIMARY_COLOR, fg_color=TEXT_LIGHT, width=20):
    """Tạo button với style đẹp"""
    btn = tk.Button(parent, text=text, command=command, 
                   bg=bg_color, fg=fg_color, 
                   font=("Segoe UI", 11, "bold"),
                   relief=tk.FLAT, bd=0,
                   padx=20, pady=10,
                   width=width,
                   cursor="hand2")
    # Hiệu ứng hover
    def on_enter(e):
        btn['bg'] = lighten_color(bg_color)
    def on_leave(e):
        btn['bg'] = bg_color
    btn.bind("<Enter>", on_enter)
    btn.bind("<Leave>", on_leave)
    return btn

def lighten_color(hex_color):
    """Làm sáng màu lên một chút cho hiệu ứng hover"""
    hex_color = hex_color.lstrip('#')
    r, g, b = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
    r = min(255, int(r * 1.2))
    g = min(255, int(g * 1.2))
    b = min(255, int(b * 1.2))
    return f'#{r:02x}{g:02x}{b:02x}'

def create_styled_frame(parent, bg_color=CARD_BG):
    """Tạo frame với style card đẹp"""
    frame = tk.Frame(parent, bg=bg_color, relief=tk.FLAT, bd=0)
    return frame

def create_styled_entry(parent, width=30):
    """Tạo entry field đẹp"""
    entry = tk.Entry(parent, width=width, 
                    font=("Segoe UI", 11),
                    relief=tk.FLAT, bd=2,
                    bg=TEXT_LIGHT,
                    fg=TEXT_DARK)
    return entry

# --- 3. GUI SETUP ---
root = tk.Tk()
root.title(" Ai Là Triệu Phú - Online Quiz Game")
root.geometry("800x600")
root.configure(bg=PRIMARY_COLOR)

# Không cho phép resize
try:
    root.resizable(False, False)
except:
    pass

# Frame Connect với style mới
frame_connect = create_styled_frame(root, PRIMARY_COLOR)
frame_connect.configure(bg=PRIMARY_COLOR)

# Title
title_label = tk.Label(frame_connect, 
                       text=" AI LÀ TRIỆU PHÚ",
                       font=("Segoe UI", 28, "bold"),
                       fg=TEXT_LIGHT,
                       bg=PRIMARY_COLOR)
title_label.pack(pady=30)

subtitle_label = tk.Label(frame_connect,
                         text="Kết nối đến máy chủ",
                         font=("Segoe UI", 14),
                         fg=WARNING_COLOR,
                         bg=PRIMARY_COLOR)
subtitle_label.pack(pady=10)

# Card chứa form connect
connect_card = create_styled_frame(frame_connect, CARD_BG)
connect_card.pack(pady=20, padx=100, fill=tk.BOTH)

tk.Label(connect_card, text=" Địa chỉ IP", 
        font=("Segoe UI", 11, "bold"),
        bg=CARD_BG, fg=TEXT_DARK).pack(pady=(20, 5))
entry_ip = create_styled_entry(connect_card)
entry_ip.insert(0, "127.0.0.1")
entry_ip.pack(pady=5)

tk.Label(connect_card, text=" Cổng (Port)",
        font=("Segoe UI", 11, "bold"),
        bg=CARD_BG, fg=TEXT_DARK).pack(pady=(15, 5))
entry_port = create_styled_entry(connect_card)
entry_port.insert(0, "5555")
entry_port.pack(pady=5)

create_styled_button(connect_card, " Kết nối", btn_connect_click, SUCCESS_COLOR).pack(pady=20)

# Footer thông tin
footer_label = tk.Label(frame_connect,
                       text=" Game Quiz trực tuyến - Thách đấu với bạn bè!\nPhát triển bởi Nhóm Lập Trình Mạng",
                       font=("Segoe UI", 9),
                       fg="#8d99ae",
                       bg=PRIMARY_COLOR)
footer_label.pack(side=tk.BOTTOM, pady=10)

frame_connect.pack(fill=tk.BOTH, expand=True)

frame_login = create_styled_frame(root, PRIMARY_COLOR)
frame_login.configure(bg=PRIMARY_COLOR)

# Title
title_login = tk.Label(frame_login,
                       text=" AI LÀ TRIỆU PHÚ",
                       font=("Segoe UI", 28, "bold"),
                       fg=TEXT_LIGHT,
                       bg=PRIMARY_COLOR)
title_login.pack(pady=30)

subtitle_login = tk.Label(frame_login,
                         text="Đăng nhập hoặc Đăng ký tài khoản",
                         font=("Segoe UI", 13),
                         fg=WARNING_COLOR,
                         bg=PRIMARY_COLOR)
subtitle_login.pack(pady=10)

# Card đăng nhập
login_card = create_styled_frame(frame_login, CARD_BG)
login_card.pack(pady=20, padx=100, fill=tk.BOTH)

tk.Label(login_card, text=" Tên đăng nhập",
        font=("Segoe UI", 11, "bold"),
        bg=CARD_BG, fg=TEXT_DARK).pack(pady=(20, 5))
entry_user = create_styled_entry(login_card)
entry_user.pack(pady=5)

tk.Label(login_card, text=" Mật khẩu",
        font=("Segoe UI", 11, "bold"),
        bg=CARD_BG, fg=TEXT_DARK).pack(pady=(15, 5))
entry_pass = create_styled_entry(login_card)
entry_pass.config(show="*")
entry_pass.pack(pady=5)

# Frame chứa các nút
btn_frame = tk.Frame(login_card, bg=CARD_BG)
btn_login_btn = create_styled_button(btn_frame, " Đăng nhập", btn_login_click, SUCCESS_COLOR, width=12)
btn_login_btn.pack(side=tk.LEFT, padx=5)
btn_register_btn = create_styled_button(btn_frame, " Đăng ký", btn_register_click, ACCENT_COLOR, width=12)
btn_register_btn.pack(side=tk.LEFT, padx=5)
btn_frame.pack(pady=20)

# Frame Lobby với style mới
frame_lobby = create_styled_frame(root, PRIMARY_COLOR)
frame_lobby.configure(bg=PRIMARY_COLOR)

# Header
lobby_header = tk.Frame(frame_lobby, bg=PRIMARY_COLOR)
lobby_header.pack(fill=tk.X, pady=10)

lbl_welcome = tk.Label(lobby_header, text="...",
                       font=("Segoe UI", 18, "bold"),
                       fg=TEXT_LIGHT,
                       bg=PRIMARY_COLOR)
lbl_welcome.pack()

status_label = tk.Label(lobby_header,
                       text="🟢 Đang online",
                       font=("Segoe UI", 11),
                       fg=SUCCESS_COLOR,
                       bg=PRIMARY_COLOR)
status_label.pack()

# Card danh sách người chơi
players_card = create_styled_frame(frame_lobby, CARD_BG)
players_card.pack(pady=20, padx=50, fill=tk.BOTH, expand=True)

tk.Label(players_card,
        text=" Tất cả người chơi",
        font=("Segoe UI", 14, "bold"),
        bg=CARD_BG, fg=TEXT_DARK).pack(pady=10)

tk.Label(players_card,
        text="Chọn người chơi đang rảnh để thách đấu",
        font=("Segoe UI", 10),
        bg=CARD_BG, fg="gray").pack()

# Chú thích trạng thái
legend_frame = tk.Frame(players_card, bg=CARD_BG)
legend_frame.pack(pady=5)
tk.Label(legend_frame, text="🟢 Rảnh (Xanh)  |   Đang chơi (Đỏ)  |   Offline (Xám)",
        font=("Segoe UI", 9, "bold"),
        bg=CARD_BG,
        fg="#555").pack()

# Custom player list với màu sắc cho từng trạng thái
list_frame = tk.Frame(players_card, bg=CARD_BG)
list_frame.pack(pady=10, fill=tk.BOTH, expand=True)

# Canvas để scroll
list_canvas = tk.Canvas(list_frame, bg=TEXT_LIGHT, height=200, highlightthickness=0)
list_scrollbar = tk.Scrollbar(list_frame, orient="vertical", command=list_canvas.yview)

# Gán vào biến global
list_players_container = tk.Frame(list_canvas, bg=TEXT_LIGHT)

list_players_container.bind(
    "<Configure>",
    lambda e: list_canvas.configure(scrollregion=list_canvas.bbox("all"))
)

list_canvas.create_window((0, 0), window=list_players_container, anchor="nw")
list_canvas.configure(yscrollcommand=list_scrollbar.set)

# Hàm bind scroll chuột đệ quy cho tất cả widget con
def bind_mousewheel_recursively(widget, canvas_to_scroll):
    def on_mousewheel(event):
        canvas_to_scroll.yview_scroll(int(-1*(event.delta/120)), "units")
    
    widget.bind("<MouseWheel>", on_mousewheel)
    for child in widget.winfo_children():
        bind_mousewheel_recursively(child, canvas_to_scroll)

# Bind cho canvas và container
list_canvas.bind("<MouseWheel>", lambda e: list_canvas.yview_scroll(int(-1*(e.delta/120)), "units"))
bind_mousewheel_recursively(list_players_container, list_canvas)

list_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=20)
list_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

create_styled_button(players_card, " Làm mới", refresh_lobby, SECONDARY_COLOR, width=15).pack(pady=10)

# Action buttons
frame_actions = tk.Frame(frame_lobby, bg=PRIMARY_COLOR)
create_styled_button(frame_actions, "THACH DAU", btn_invite_click, ACCENT_COLOR, width=15).pack(side=tk.LEFT, padx=10)
create_styled_button(frame_actions, "Lich su", lambda: show_history(), WARNING_COLOR, TEXT_DARK, width=15).pack(side=tk.LEFT, padx=10)
create_styled_button(frame_actions, "Dang xuat", btn_logout_click, DANGER_COLOR, width=15).pack(side=tk.LEFT, padx=10)
frame_actions.pack(pady=20)

# Frame Game (Mới)
frame_game = tk.Frame(root)

# Các widget game - sẽ được tạo bởi setup_game_widgets()
lbl_opponent = None
lbl_scores = None
lbl_timer = None
lbl_question = None
lbl_result = None
frame_answers = None
btn_a = None
btn_b = None
btn_c = None
btn_d = None

def setup_game_widgets():
    """Tạo hoặc tạo lại các widget cho màn hình game"""
    global lbl_opponent, lbl_scores, lbl_timer, lbl_question, lbl_result
    global frame_answers, btn_a, btn_b, btn_c, btn_d
    
    # Xóa tất cả widget cũ trong frame_game
    for widget in frame_game.winfo_children():
        widget.destroy()
    
    frame_game.configure(bg=PRIMARY_COLOR)
    
    # Header game
    game_header = create_styled_frame(frame_game, SECONDARY_COLOR)
    game_header.pack(fill=tk.X, pady=10)
    
    lbl_opponent = tk.Label(game_header, text=" Đối thủ: ...",
                           font=("Segoe UI", 16, "bold"),
                           fg=ACCENT_COLOR,
                           bg=SECONDARY_COLOR)
    lbl_opponent.pack(pady=5)

    lbl_scores = tk.Label(game_header, text="Bạn: 0 | Đối thủ: 0",
                         font=("Segoe UI", 14, "bold"),
                         fg=TEXT_LIGHT,
                         bg=SECONDARY_COLOR)
    lbl_scores.pack(pady=3)

    lbl_timer = tk.Label(game_header, text="⏱ Thời gian: 15s",
                        font=("Segoe UI", 13, "bold"),
                        fg=WARNING_COLOR,
                        bg=SECONDARY_COLOR)
    lbl_timer.pack(pady=5)

    # Card câu hỏi
    question_card = create_styled_frame(frame_game, CARD_BG)
    question_card.pack(pady=15, padx=30, fill=tk.BOTH)
    
    lbl_question = tk.Label(question_card, text="Câu hỏi...",
                           font=("Segoe UI", 15, "bold"),
                           wraplength=700,
                           bg=CARD_BG,
                           fg=TEXT_DARK,
                           justify=tk.LEFT)
    lbl_question.pack(pady=20, padx=20)

    # Label hiển thị kết quả
    lbl_result = tk.Label(frame_game, text="",
                         font=("Segoe UI", 14, "bold"),
                         height=2,
                         bg=PRIMARY_COLOR)
    lbl_result.pack(pady=5)

    # Frame các đáp án với style đẹp
    frame_answers = tk.Frame(frame_game, bg=PRIMARY_COLOR)
    
    # Tạo các nút đáp án với style hiện đại
    btn_a = tk.Button(frame_answers, text="A",
                     font=("Segoe UI", 12, "bold"),
                     bg="#3a86ff", fg=TEXT_LIGHT,
                     width=30, height=2,
                     relief=tk.FLAT, bd=0,
                     cursor="hand2",
                     command=lambda: btn_answer_click(0))
    btn_a.grid(row=0, column=0, padx=10, pady=8)
    
    btn_b = tk.Button(frame_answers, text="B",
                     font=("Segoe UI", 12, "bold"),
                     bg="#8338ec", fg=TEXT_LIGHT,
                     width=30, height=2,
                     relief=tk.FLAT, bd=0,
                     cursor="hand2",
                     command=lambda: btn_answer_click(1))
    btn_b.grid(row=0, column=1, padx=10, pady=8)
    
    btn_c = tk.Button(frame_answers, text="C",
                     font=("Segoe UI", 12, "bold"),
                     bg="#ff006e", fg=TEXT_LIGHT,
                     width=30, height=2,
                     relief=tk.FLAT, bd=0,
                     cursor="hand2",
                     command=lambda: btn_answer_click(2))
    btn_c.grid(row=1, column=0, padx=10, pady=8)
    
    btn_d = tk.Button(frame_answers, text="D",
                     font=("Segoe UI", 12, "bold"),
                     bg="#fb5607", fg=TEXT_LIGHT,
                     width=30, height=2,
                     relief=tk.FLAT, bd=0,
                     cursor="hand2",
                     command=lambda: btn_answer_click(3))
    btn_d.grid(row=1, column=1, padx=10, pady=8)
    
    frame_answers.pack(pady=10)

    create_styled_button(frame_game, " Dừng cuộc chơi", quit_game, DANGER_COLOR, width=18).pack(pady=20)

# Khởi tạo các widget game lần đầu
setup_game_widgets()

# Frame Lịch sử đấu
frame_history = tk.Frame(root)

def show_history():
    """Hiển thị lịch sử các trận đấu"""
    frame_lobby.pack_forget()
    frame_history.pack(fill=tk.BOTH, expand=True)
    
    # Xóa nội dung cũ
    for widget in frame_history.winfo_children():
        widget.destroy()
    
    frame_history.configure(bg=PRIMARY_COLOR)
    
    # Header
    tk.Label(frame_history, text=" LỊCH SỬ ĐẤU",
            font=("Segoe UI", 24, "bold"),
            fg=TEXT_LIGHT,
            bg=PRIMARY_COLOR).pack(pady=20)
    
    # Lấy dữ liệu lịch sử từ server
    res = send_json({"type": "GET_HISTORY"})
    
    if res.get("type") == "HISTORY_DATA":
        history_list = res.get("history", [])
        
        if len(history_list) == 0:
            empty_card = create_styled_frame(frame_history, CARD_BG)
            empty_card.pack(pady=40, padx=100)
            tk.Label(empty_card, text=" Chưa có trận đấu nào!",
                    font=("Segoe UI", 14),
                    bg=CARD_BG,
                    fg="gray").pack(pady=40, padx=60)
        else:
            # Tạo canvas với scrollbar
            canvas = tk.Canvas(frame_history, height=400, bg=PRIMARY_COLOR, highlightthickness=0)
            scrollbar = tk.Scrollbar(frame_history, orient="vertical", command=canvas.yview)
            scrollable_frame = tk.Frame(canvas, bg=PRIMARY_COLOR)
            
            scrollable_frame.bind(
                "<Configure>",
                lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
            )
            
            canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
            canvas.configure(yscrollcommand=scrollbar.set)
            
            # Bind scroll chuột cho canvas lịch sử
            def on_history_mousewheel(event):
                canvas.yview_scroll(int(-1*(event.delta/120)), "units")
            
            canvas.bind("<MouseWheel>", on_history_mousewheel)
            scrollable_frame.bind("<MouseWheel>", on_history_mousewheel)
            
            # Hiển thị từng trận đấu (sắp xếp mới nhất lên trên)
            history_list.reverse()
            for idx, game in enumerate(history_list):
                game_key = int(game.get("game_key", 0))
                player1 = game.get("player1")
                player2 = game.get("player2")
                score1 = game.get("score1")
                score2 = game.get("score2")
                total_q = game.get("total_questions")
                result = game.get("result")  # WIN/LOSE/DRAW
                timestamp = game.get("timestamp")
                
                # Format thời gian
                import datetime
                dt = datetime.datetime.fromtimestamp(timestamp)
                time_str = dt.strftime("%Y-%m-%d %H:%M:%S")
                
                # Frame cho mỗi trận
                match_bg = "#e8f5e9" if result == "WIN" else "#ffebee" if result == "LOSE" else "#fff3e0"
                match_frame = tk.Frame(scrollable_frame, relief=tk.FLAT, borderwidth=1, bg=match_bg)
                match_frame.pack(fill=tk.BOTH, padx=10, pady=5)
                match_frame.bind("<MouseWheel>", on_history_mousewheel)
                
                # Tiêu đề
                result_color = SUCCESS_COLOR if result == "WIN" else DANGER_COLOR if result == "LOSE" else WARNING_COLOR
                result_text = " THẮNG" if result == "WIN" else " THUA" if result == "LOSE" else " HÒA"
                
                title_label = tk.Label(match_frame, text=f"Trận #{len(history_list) - idx}: {result_text}", 
                        font=("Segoe UI", 13, "bold"),
                        fg=result_color,
                        bg=match_bg)
                title_label.pack(anchor="w", padx=10, pady=5)
                title_label.bind("<MouseWheel>", on_history_mousewheel)
                
                # Thông tin chi tiết
                opponent_label = tk.Label(match_frame, text=f" Đối thủ: {player2 if player1 == current_user else player1}", 
                        font=("Segoe UI", 11),
                        bg=match_bg,
                        fg=TEXT_DARK)
                opponent_label.pack(anchor="w", padx=10)
                opponent_label.bind("<MouseWheel>", on_history_mousewheel)
                
                score_label = tk.Label(match_frame, text=f" Tỉ số: {score1} - {score2} | {total_q} câu hỏi", 
                        font=("Segoe UI", 10, "bold"),
                        bg=match_bg,
                        fg=TEXT_DARK)
                score_label.pack(anchor="w", padx=10)
                score_label.bind("<MouseWheel>", on_history_mousewheel)
                
                time_label = tk.Label(match_frame, text=f" {time_str}", 
                        font=("Segoe UI", 9),
                        fg="gray",
                        bg=match_bg)
                time_label.pack(anchor="w", padx=10)
                time_label.bind("<MouseWheel>", on_history_mousewheel)
                
                id_label = tk.Label(match_frame, text=f"Game ID: {game_key}", 
                        font=("Segoe UI", 8),
                        fg="darkgray",
                        bg=match_bg)
                id_label.pack(anchor="w", padx=10, pady=(0, 5))
                id_label.bind("<MouseWheel>", on_history_mousewheel)
            
            canvas.pack(side="left", fill="both", expand=True)
            scrollbar.pack(side="right", fill="y")
    else:
        error_card = create_styled_frame(frame_history, CARD_BG)
        error_card.pack(pady=40, padx=100)
        tk.Label(error_card, text=" Không thể lấy dữ liệu!",
                font=("Segoe UI", 14),
                bg=CARD_BG,
                fg=DANGER_COLOR).pack(pady=40, padx=60)
    
    # Nút quay lại
    create_styled_button(frame_history, " Quay lại Lobby", 
                        lambda: (frame_history.pack_forget(), frame_lobby.pack()),
                        SUCCESS_COLOR, width=20).pack(pady=20)

root.mainloop()
