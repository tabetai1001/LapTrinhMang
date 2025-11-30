import tkinter as tk
from tkinter import messagebox, ttk
from ctypes import *
import json
import time

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
    except: return
    if lib.connect_to_server(ip.encode('utf-8'), port):
        frame_connect.pack_forget()
        frame_login.pack()
    else: messagebox.showerror("Lỗi", "Kết nối thất bại")

def btn_login_click():
    global current_user
    user = entry_user.get(); pwd = entry_pass.get()
    res = send_json({"type": "LOGIN", "user": user, "pass": pwd})
    if res.get("type") == "LOGIN_SUCCESS":
        current_user = res.get("user")
        lbl_welcome.config(text=f"Xin chào: {current_user}")
        frame_login.pack_forget(); frame_lobby.pack()
        refresh_lobby()
        # BẮT ĐẦU VÒNG LẶP POLLING (Hỏi vòng)
        root.after(1000, poll_server) 
    else: messagebox.showerror("Lỗi", res.get("message"))

def btn_register_click():
    user = entry_user.get(); pwd = entry_pass.get()
    if not user or not pwd:
        messagebox.showwarning("Chú ý", "Vui lòng nhập đầy đủ thông tin!")
        return
    
    res = send_json({"type": "REGISTER", "user": user, "pass": pwd})
    if res.get("type") == "REGISTER_SUCCESS":
        messagebox.showinfo("Thành công", res.get("message"))
        entry_user.delete(0, tk.END)
        entry_pass.delete(0, tk.END)
    else:
        messagebox.showerror("Lỗi", res.get("message"))

def refresh_lobby():
    global last_player_list
    res = send_json({"type": "GET_LOBBY_LIST"})
    current_players = res.get("players", [])
    
    # Chỉ cập nhật nếu có thay đổi
    if current_players != last_player_list:
        # Lưu lựa chọn hiện tại (nếu có)
        current_selection = None
        if list_players.curselection():
            current_selection = list_players.get(list_players.curselection()[0])
        
        # Cập nhật danh sách
        list_players.delete(0, tk.END)
        for p in current_players:
            list_players.insert(tk.END, p)
        
        # Khôi phục lựa chọn nếu người đó vẫn còn online
        if current_selection and current_selection in current_players:
            idx = current_players.index(current_selection)
            list_players.selection_set(idx)
        
        last_player_list = current_players.copy()

# --- CHỨC NĂNG MỚI: MỜI THÁCH ĐẤU ---
def btn_invite_click():
    selection = list_players.curselection()
    if not selection:
        messagebox.showwarning("Chú ý", "Hãy chọn một người chơi để thách đấu")
        return
    target = list_players.get(selection[0])
    
    # Popup chọn số câu hỏi
    choice_window = tk.Toplevel(root)
    choice_window.title("Chọn số câu hỏi")
    choice_window.geometry("300x200")
    
    tk.Label(choice_window, text=f"Thách đấu với {target}", font=("Arial", 12, "bold")).pack(pady=10)
    tk.Label(choice_window, text="Chọn số câu hỏi:").pack(pady=5)
    
    num_q_var = tk.IntVar(value=5)
    tk.Radiobutton(choice_window, text="5 câu (Nhanh)", variable=num_q_var, value=5).pack()
    tk.Radiobutton(choice_window, text="10 câu (Trung bình)", variable=num_q_var, value=10).pack()
    tk.Radiobutton(choice_window, text="15 câu (Dài)", variable=num_q_var, value=15).pack()
    
    def send_invite():
        num_questions = num_q_var.get()
        choice_window.destroy()
        
        # Gửi lời mời
        res = send_json({"type": "INVITE_PLAYER", "target": target, "num_questions": num_questions})
        if res.get("type") == "INVITE_SENT_SUCCESS":
            messagebox.showinfo("Đã gửi", f"Đang chờ {target} trả lời ({num_questions} câu)...")
        else:
            messagebox.showerror("Lỗi", res.get("message"))
    
    tk.Button(choice_window, text="Gửi lời mời", bg="green", fg="white", command=send_invite).pack(pady=20)

# --- CHỨC NĂNG MỚI: POLLING LOOP ---
def poll_server():
    if is_in_game: 
        # Đang chơi thì không poll ở đây (xử lý riêng)
        root.after(1000, poll_server)  # Vẫn lặp lại để sẵn sàng khi về lobby
        return
    
    # Tự động cập nhật danh sách người chơi online (CHỎ KHI KHÔNG ĐANG CHƠI)
    try:
        refresh_lobby()
    except:
        pass  # Tránh lỗi khi frame chưa được hiển thị
    
    # Gửi gói tin POLL để hỏi Server có gì mới không
    res = send_json({"type": "POLL"})
    msg_type = res.get("type")
    
    if msg_type == "RECEIVE_INVITE":
        inviter = res.get("from")
        num_q = res.get("num_questions", 5)
        # Hiện Popup hỏi ý kiến
        ans = messagebox.askyesno("Thách đấu!", f"Người chơi {inviter} muốn thách đấu bạn?\nSố câu hỏi: {num_q}")
        if ans:
            # Đồng ý
            accept_res = send_json({"type": "ACCEPT_INVITE", "from": inviter})
            if not is_in_game:  # Kiểm tra lại trước khi start
                game_key = int(accept_res.get("game_key", 0))
                start_game(inviter, game_key)
        else:
            # Từ chối
            send_json({"type": "REJECT_INVITE", "from": inviter})
            
    elif msg_type == "GAME_START":
        opponent = res.get("opponent")
        game_key = int(res.get("game_key", 0))
        if not is_in_game:  # Kiểm tra lại trước khi start để tránh start 2 lần
            start_game(opponent, game_key)
        
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
        messagebox.showinfo("Kết thúc", "Đã hết câu hỏi!")
        quit_game()
    elif res.get("type") == "ERROR":
        messagebox.showerror("Lỗi", f"Lỗi: {res.get('message', 'Unknown error')}")
        quit_game()
    else:
        messagebox.showerror("Lỗi", f"Phản hồi không xác định: {res.get('type')}")
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
    
    lbl_question.config(text=f"Câu {q_num}/{total}: {question_text}")
    
    if len(options) >= 4:
        btn_a.config(text=f"A. {options[0]}", state=tk.NORMAL)
        btn_b.config(text=f"B. {options[1]}", state=tk.NORMAL)
        btn_c.config(text=f"C. {options[2]}", state=tk.NORMAL)
        btn_d.config(text=f"D. {options[3]}", state=tk.NORMAL)
    else:
        messagebox.showerror("Lỗi", f"Không đủ đáp án! Chỉ có {len(options)} đáp án")

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
            btn.config(bg="green", fg="white")
        elif i == ans_index and not is_correct:
            btn.config(bg="red", fg="white")
        elif ans_index == -1:  # Timeout: đáp án đúng cũng đỏ
            btn.config(bg="red", fg="white")
        else:
            btn.config(bg="lightgray")
    
    # Hiển thị thông báo kết quả trong label
    if ans_index == -1:
        lbl_result.config(text="⏰ HẾT GIỜ!", fg="red", font=("Arial", 14, "bold"))
    elif is_correct:
        lbl_result.config(text=f"✓ ĐÚNG! +{earned} điểm", fg="green", font=("Arial", 14, "bold"))
    else:
        lbl_result.config(text=f"✗ SAI! Đáp án đúng: {chr(65 + correct_ans)}", fg="red", font=("Arial", 14, "bold"))
    
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
    
    # Tiêu đề kết quả
    result_label = tk.Label(frame_game, 
        text="🎉 CHIẾN THẮNG! 🎉" if you_win else "😢 THUA CUỘC 😢",
        font=("Arial", 24, "bold"),
        fg="green" if you_win else "red")
    result_label.pack(pady=20)
    
    # Điểm số
    score_frame = tk.Frame(frame_game)
    score_frame.pack(pady=10)
    tk.Label(score_frame, text=f"Điểm của bạn: {final_my_score}", font=("Arial", 16, "bold")).pack()
    tk.Label(score_frame, text=f"Điểm đối thủ ({current_opponent}): {final_opp_score}", font=("Arial", 16, "bold")).pack()
    
    # Thời gian trả lời trung bình
    if len(game_history) > 0:
        avg_time = sum(h["time_taken"] for h in game_history) / len(game_history)
        tk.Label(score_frame, text=f"Thời gian TB: {avg_time:.2f}s", font=("Arial", 12)).pack()
    
    # Lịch sử câu hỏi
    tk.Label(frame_game, text="Chi tiết các câu hỏi:", font=("Arial", 14, "bold")).pack(pady=10)
    
    # Frame cuộn để hiển thị lịch sử
    canvas = tk.Canvas(frame_game, height=250)
    scrollbar = tk.Scrollbar(frame_game, orient="vertical", command=canvas.yview)
    scrollable_frame = tk.Frame(canvas)
    
    scrollable_frame.bind(
        "<Configure>",
        lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
    )
    
    canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
    canvas.configure(yscrollcommand=scrollbar.set)
    
    # Hiển thị từng câu hỏi
    for i, h in enumerate(game_history):
        q_frame = tk.Frame(scrollable_frame, relief=tk.RAISED, borderwidth=2)
        q_frame.pack(fill=tk.BOTH, padx=10, pady=5)
        
        tk.Label(q_frame, text=f"Câu {i+1}: {h['question']}", font=("Arial", 10, "bold"), wraplength=600).pack(anchor="w")
        
        for j, opt in enumerate(h['options']):
            color = "black"
            prefix = chr(65 + j)
            
            # Xử lý timeout: tất cả đáp án đều đỏ
            if h.get('is_timeout', False):
                color = "red"
                if j == h['correct_answer']:
                    prefix += " (Đúng)"
            elif j == h['correct_answer']:
                color = "green"
                prefix += " ✓"
            elif j == h['my_answer'] and j != h['correct_answer']:
                color = "red"
                prefix += " ✗"
            elif j == h['my_answer']:
                prefix += " ✓"
            
            tk.Label(q_frame, text=f"{prefix}. {opt}", fg=color).pack(anchor="w", padx=20)
        
        # Hiển thị thời gian với cảnh báo nếu timeout
        time_text = f"Thời gian: {h['time_taken']:.2f}s"
        if h.get('is_timeout', False):
            time_text += " ⏰ HẾT GIỜ"
        tk.Label(q_frame, text=time_text, font=("Arial", 9), fg="red" if h.get('is_timeout', False) else "black").pack(anchor="w", padx=20)
    
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
    
    tk.Button(frame_game, text="Về Lobby", bg="blue", fg="white", command=close_result, font=("Arial", 14, "bold")).pack(pady=10)

def quit_game():
    global is_in_game, game_history, current_opponent, current_game_key
    print(f"[Client] Quitting game mid-match. GAME_KEY={current_game_key}")
    
    is_in_game = False  # Reset trước
    game_history = []  # Clear history khi thoát giữa chừng
    current_opponent = ""  # Reset opponent
    current_game_key = 0  # Reset game_key
    
    # Gửi thông báo thoát game để reset trạng thái server
    send_json({"type": "QUIT_GAME"})
    
    frame_game.pack_forget()
    frame_lobby.pack()
    
    # Cập nhật danh sách người chơi
    try:
        refresh_lobby()
    except Exception as e:
        print(f"[Client] Error refreshing lobby: {e}")
    
    # KHÔNG gọi root.after(1000, poll_server) vì nó đã tự lặp lại

# --- 3. GUI SETUP ---
root = tk.Tk(); root.title("Ai Là Triệu Phú"); root.geometry("600x500")

# Frame Connect & Login (Giữ nguyên)
frame_connect = tk.Frame(root)
tk.Label(frame_connect, text="IP").pack(); entry_ip = tk.Entry(frame_connect); entry_ip.insert(0,"127.0.0.1"); entry_ip.pack()
tk.Label(frame_connect, text="Port").pack(); entry_port = tk.Entry(frame_connect); entry_port.insert(0,"5555"); entry_port.pack()
tk.Button(frame_connect, text="Connect", command=btn_connect_click).pack()
frame_connect.pack(pady=50)

frame_login = tk.Frame(root)
tk.Label(frame_login, text="ĐĂNG NHẬP / ĐĂNG KÝ", font=("Arial", 14, "bold")).pack(pady=10)
tk.Label(frame_login, text="Username").pack()
entry_user = tk.Entry(frame_login, width=30)
entry_user.pack(pady=5)
tk.Label(frame_login, text="Password").pack()
entry_pass = tk.Entry(frame_login, show="*", width=30)
entry_pass.pack(pady=5)

# Frame chứa các nút
btn_frame = tk.Frame(frame_login)
tk.Button(btn_frame, text="Đăng nhập", bg="green", fg="white", width=12, command=btn_login_click).pack(side=tk.LEFT, padx=5)
tk.Button(btn_frame, text="Đăng ký", bg="blue", fg="white", width=12, command=btn_register_click).pack(side=tk.LEFT, padx=5)
btn_frame.pack(pady=10)

# Frame Lobby (Cập nhật nút Thách đấu)
frame_lobby = tk.Frame(root)
lbl_welcome = tk.Label(frame_lobby, text="...", font=("Arial", 12, "bold")); lbl_welcome.pack(pady=10)
tk.Label(frame_lobby, text="Người chơi online (Chọn để thách đấu):").pack()
list_players = tk.Listbox(frame_lobby, height=5); list_players.pack()
tk.Button(frame_lobby, text="Làm mới", command=refresh_lobby).pack()

frame_actions = tk.Frame(frame_lobby)
tk.Button(frame_actions, text="THÁCH ĐẤU (PvP)", bg="orange", command=btn_invite_click).pack(side=tk.LEFT, padx=5)
tk.Button(frame_actions, text="Lịch sử đấu", bg="blue", fg="white", command=lambda: show_history()).pack(side=tk.LEFT, padx=5)
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
    
    # Tạo lại các widget
    lbl_opponent = tk.Label(frame_game, text="Đối thủ: ...", fg="red", font=("Arial", 12, "bold"))
    lbl_opponent.pack(pady=5)

    lbl_scores = tk.Label(frame_game, text="Bạn: 0 | Đối thủ: 0", font=("Arial", 12, "bold"))
    lbl_scores.pack(pady=2)

    lbl_timer = tk.Label(frame_game, text="Thời gian: 15s", font=("Arial", 12), fg="blue")
    lbl_timer.pack(pady=5)

    lbl_question = tk.Label(frame_game, text="Câu hỏi...", font=("Arial", 14), wraplength=500)
    lbl_question.pack(pady=15)

    # Label hiển thị kết quả (đúng/sai)
    lbl_result = tk.Label(frame_game, text="", font=("Arial", 12, "bold"), height=2)
    lbl_result.pack(pady=5)

    frame_answers = tk.Frame(frame_game)
    btn_a = tk.Button(frame_answers, text="A", width=25, height=2, command=lambda: btn_answer_click(0))
    btn_a.grid(row=0, column=0, padx=5, pady=5)
    btn_b = tk.Button(frame_answers, text="B", width=25, height=2, command=lambda: btn_answer_click(1))
    btn_b.grid(row=0, column=1, padx=5, pady=5)
    btn_c = tk.Button(frame_answers, text="C", width=25, height=2, command=lambda: btn_answer_click(2))
    btn_c.grid(row=1, column=0, padx=5, pady=5)
    btn_d = tk.Button(frame_answers, text="D", width=25, height=2, command=lambda: btn_answer_click(3))
    btn_d.grid(row=1, column=1, padx=5, pady=5)
    frame_answers.pack()

    tk.Button(frame_game, text="Dừng cuộc chơi", bg="gray", command=quit_game).pack(pady=20)

# Khởi tạo các widget game lần đầu
setup_game_widgets()

# Frame Lịch sử đấu
frame_history = tk.Frame(root)

def show_history():
    """Hiển thị lịch sử các trận đấu"""
    frame_lobby.pack_forget()
    frame_history.pack()
    
    # Xóa nội dung cũ
    for widget in frame_history.winfo_children():
        widget.destroy()
    
    tk.Label(frame_history, text="LỊCH SỬ ĐẤU", font=("Arial", 18, "bold"), fg="blue").pack(pady=10)
    
    # Lấy dữ liệu lịch sử từ server
    res = send_json({"type": "GET_HISTORY"})
    
    if res.get("type") == "HISTORY_DATA":
        history_list = res.get("history", [])
        
        if len(history_list) == 0:
            tk.Label(frame_history, text="Chưa có trận đấu nào!", font=("Arial", 12)).pack(pady=20)
        else:
            # Tạo canvas với scrollbar
            canvas = tk.Canvas(frame_history, height=400)
            scrollbar = tk.Scrollbar(frame_history, orient="vertical", command=canvas.yview)
            scrollable_frame = tk.Frame(canvas)
            
            scrollable_frame.bind(
                "<Configure>",
                lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
            )
            
            canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
            canvas.configure(yscrollcommand=scrollbar.set)
            
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
                match_frame = tk.Frame(scrollable_frame, relief=tk.RAISED, borderwidth=2, bg="lightyellow")
                match_frame.pack(fill=tk.BOTH, padx=10, pady=5)
                
                # Tiêu đề
                result_color = "green" if result == "WIN" else "red" if result == "LOSE" else "orange"
                result_text = "🏆 THẮNG" if result == "WIN" else "😢 THUA" if result == "LOSE" else "🤝 HÒA"
                
                tk.Label(match_frame, text=f"Trận #{len(history_list) - idx}: {result_text}", 
                        font=("Arial", 12, "bold"), fg=result_color, bg="lightyellow").pack(anchor="w", padx=5)
                
                # Thông tin chi tiết
                tk.Label(match_frame, text=f"Đối thủ: {player2 if player1 == current_user else player1}", 
                        font=("Arial", 10), bg="lightyellow").pack(anchor="w", padx=5)
                tk.Label(match_frame, text=f"Tỉ số: {score1} - {score2} ({total_q} câu hỏi)", 
                        font=("Arial", 10), bg="lightyellow").pack(anchor="w", padx=5)
                tk.Label(match_frame, text=f"Thời gian: {time_str}", 
                        font=("Arial", 9), fg="gray", bg="lightyellow").pack(anchor="w", padx=5)
                tk.Label(match_frame, text=f"ID: {game_key}", 
                        font=("Arial", 8), fg="darkgray", bg="lightyellow").pack(anchor="w", padx=5)
            
            canvas.pack(side="left", fill="both", expand=True)
            scrollbar.pack(side="right", fill="y")
    else:
        tk.Label(frame_history, text="Không thể lấy dữ liệu!", font=("Arial", 12)).pack(pady=20)
    
    # Nút quay lại
    tk.Button(frame_history, text="Quay lại Lobby", bg="green", fg="white", 
             command=lambda: (frame_history.pack_forget(), frame_lobby.pack())).pack(pady=10)

root.mainloop()