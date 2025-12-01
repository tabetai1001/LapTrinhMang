import tkinter as tk
import time
from tkinter import messagebox
from ui.widgets import *
from core.config import *

class GameView(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent, bg=BG_PRIMARY)
        self.controller = controller
        
        # Game State
        self.mode = "PVP" 
        self.opponent = ""
        
        # Timer variables
        self.timer_id = None
        self.time_left = 0
        
        self.current_q_id = 0
        self.my_score = 0
        self.opp_score = 0
        self.lifelines_used = []
        
        self.setup_ui()

    def setup_ui(self):
        # --- HEADER (Info + Timer) ---
        self.header = tk.Frame(self, bg=BG_SECONDARY)
        self.header.pack(fill=tk.X, pady=10)
        
        self.lbl_info = tk.Label(self.header, text="", font=("Segoe UI", 14, "bold"), fg="white", bg=BG_SECONDARY)
        self.lbl_info.pack(pady=5)
        
        self.lbl_timer = tk.Label(self.header, text="", font=("Segoe UI", 14, "bold"), fg=WARNING_COLOR, bg=BG_SECONDARY)
        self.lbl_timer.pack(pady=5)
        
        # --- LIFELINE BAR (Trợ giúp) ---
        self.lifeline_frame = tk.Frame(self, bg=BG_PRIMARY)
        # Luôn tạo khung, sẽ pack/unpack khi start game
        
        # Tạo 4 nút trợ giúp
        self.btn_5050 = create_styled_button(self.lifeline_frame, "50:50", lambda: self.use_lifeline(1), width=8, bg_color="#FF9800")
        self.btn_5050.pack(side=tk.LEFT, padx=5)
        
        self.btn_audit = create_styled_button(self.lifeline_frame, "Khán giả", lambda: self.use_lifeline(2), width=8, bg_color="#2196F3")
        self.btn_audit.pack(side=tk.LEFT, padx=5)
        
        self.btn_call = create_styled_button(self.lifeline_frame, "Gọi điện", lambda: self.use_lifeline(3), width=8, bg_color="#9C27B0")
        self.btn_call.pack(side=tk.LEFT, padx=5)
        
        self.btn_swap = create_styled_button(self.lifeline_frame, "Đổi câu", lambda: self.use_lifeline(4), width=8, bg_color="#009688")
        self.btn_swap.pack(side=tk.LEFT, padx=5)

        # --- QUESTION CARD ---
        self.q_card = create_styled_frame(self, CARD_BG)
        self.q_card.pack(pady=15, padx=30, fill=tk.BOTH)
        
        self.lbl_question = tk.Label(self.q_card, text="Đang tải câu hỏi...", font=("Segoe UI", 16), wraplength=700, bg=CARD_BG, fg=TEXT_DARK)
        self.lbl_question.pack(pady=20, padx=20)
        
        # --- ANSWERS ---
        self.ans_frame = tk.Frame(self, bg=BG_PRIMARY)
        self.ans_frame.pack(pady=10)
        
        self.btn_opts = []
        for i in range(4):
            btn = tk.Button(self.ans_frame, text="", font=("Segoe UI", 12), width=30, height=2,
                           command=lambda idx=i: self.submit_answer(idx))
            btn.grid(row=i//2, column=i%2, padx=10, pady=10)
            self.btn_opts.append(btn)
            
        # --- RESULT NOTIFICATION ---
        self.lbl_result = tk.Label(self, text="", font=("Segoe UI", 14, "bold"), bg=BG_PRIMARY, fg=SUCCESS_COLOR)
        self.lbl_result.pack(pady=5)

        # --- QUIT BUTTON ---
        create_styled_button(self, "Thoát Game", self.quit_game, DANGER_COLOR, width=15).pack(pady=10)

    def start_game(self, opponent_name="Máy", mode="PVP"):
        # 1. Dọn dẹp timer cũ
        self.stop_timer() 
        
        # 2. Cập nhật trạng thái
        self.controller.is_in_game = True
        self.mode = mode
        self.opponent = opponent_name if mode == "PVP" else "BOT"
        self.my_score = 0
        self.opp_score = 0
        self.lifelines_used = []
        
        # 3. Hiển thị Info
        mode_text = "CHƠI ĐƠN" if mode == "CLASSIC" else f"ĐỐI ĐẦU: {self.opponent}"
        self.lbl_info.config(text=f"{mode_text} | Điểm: 0")
        self.lbl_result.config(text="")
        
        # 4. Reset nút Trợ giúp
        for btn in [self.btn_5050, self.btn_audit, self.btn_call, self.btn_swap]:
            btn.config(state=tk.NORMAL, bg=BG_SECONDARY) # Reset về màu mặc định

        # 5. Hiển thị khung trợ giúp (Cho CẢ 2 chế độ)
        # FIX: Luôn hiển thị lifeline_frame
        self.lifeline_frame.pack(pady=5, after=self.header)

        # 6. Bắt đầu câu hỏi
        self.request_question()

    def stop_timer(self):
        if self.timer_id:
            try:
                self.after_cancel(self.timer_id)
            except:
                pass
            self.timer_id = None

    def request_question(self):
        self.stop_timer()
        self.lbl_result.config(text="")
        
        # Reset các nút đáp án
        for btn in self.btn_opts:
            btn.config(state=tk.NORMAL, bg="white", text="", relief=tk.RAISED)
        
        # FIX: Bật lại các quyền trợ giúp chưa dùng (cho CẢ 2 chế độ)
        btn_map = {1: self.btn_5050, 2: self.btn_audit, 3: self.btn_call, 4: self.btn_swap}
        for lid, btn in btn_map.items():
            if lid not in self.lifelines_used:
                btn.config(state=tk.NORMAL, bg=BG_SECONDARY)
            else:
                btn.config(state=tk.DISABLED, bg="gray")
            
        res = self.controller.network.send_request({"type": "REQUEST_QUESTION"})
        
        if res.get("type") == "QUESTION":
            self.display_question(res)
        elif res.get("type") == "NO_MORE_QUESTIONS":
            pass 
        elif res.get("type") == "ERROR":
            messagebox.showerror("Lỗi", f"Lỗi Server: {res.get('message')}")
            self.quit_game()

    def display_question(self, data):
        self.current_q_id = data["question_id"]
        q_num = data.get("question_number", "?")
        total = data.get("total_questions", "?")
        
        self.lbl_question.config(text=f"Câu {q_num}/{total}:\n{data['question']}")
        
        score_info = f"Bạn: {self.my_score}"
        if self.mode == "PVP":
            score_info += f" | {self.opponent}: {self.opp_score}"
        self.lbl_info.config(text=score_info)
        
        opts = data["options"]
        for i, opt in enumerate(opts):
            self.btn_opts[i].config(text=f"{chr(65+i)}. {opt}")
        
        self.time_left = data.get("max_time", 15)
        self.lbl_timer.config(text=f"⏱ {self.time_left}s", fg="white")
        self.count_down()

    def count_down(self):
        if not self.controller.is_in_game: return

        if self.time_left > 0:
            color = WARNING_COLOR if self.time_left <= 5 else "white"
            self.lbl_timer.config(text=f"⏱ {self.time_left}s", fg=color)
            self.time_left -= 1
            self.timer_id = self.after(1000, self.count_down)
        else:
            self.lbl_timer.config(text="⏱ 0s", fg=DANGER_COLOR)
            self.submit_answer(-1)

    def submit_answer(self, idx):
        self.stop_timer()
        
        time_taken = 15 - self.time_left 
        if time_taken < 0: time_taken = 0
        if time_taken > 15: time_taken = 15
        
        # Khóa tất cả nút
        for btn in self.btn_opts: btn.config(state=tk.DISABLED)
        for btn in [self.btn_5050, self.btn_audit, self.btn_call, self.btn_swap]:
            btn.config(state=tk.DISABLED)
        
        res = self.controller.network.send_request({
            "type": "SUBMIT_ANSWER",
            "question_id": self.current_q_id,
            "answer_index": idx,
            "time_taken": float(time_taken)
        })
        
        if res.get("type") == "ANSWER_RESULT":
            self.show_answer_result(res, idx)

    def show_answer_result(self, res, my_ans):
        correct = res.get("correct_answer")
        self.my_score = res.get("your_total_score", 0)
        self.opp_score = res.get("opponent_score", 0)
        is_correct = res.get("is_correct")
        
        if my_ans != -1:
            if is_correct:
                self.btn_opts[my_ans].config(bg=SUCCESS_COLOR)
            else:
                self.btn_opts[my_ans].config(bg=DANGER_COLOR)
        
        if 0 <= correct < 4:
            self.btn_opts[correct].config(bg=SUCCESS_COLOR)
        
        if is_correct:
            self.lbl_result.config(text=f"CHÍNH XÁC! +{res.get('earned_score')} điểm", fg=SUCCESS_COLOR)
        else:
            msg = "HẾT GIỜ!" if my_ans == -1 else "SAI RỒI!"
            self.lbl_result.config(text=msg, fg=DANGER_COLOR)
        
        game_status = res.get("game_status")
        
        if game_status == "FINISHED":
            you_win = res.get("you_win", False)
            if self.mode == "CLASSIC":
                 self.after(2000, self.show_game_over_classic)
            else:
                 self.after(2000, lambda: self.show_game_over_pvp(you_win))
                 
        elif game_status == "WAITING_OPPONENT":
            self.lbl_result.config(text="⏳ Đang chờ đối thủ...", fg="yellow")
            self.wait_opponent()
        else:
            self.after(1500, self.request_question)

    def wait_opponent(self):
        if not self.controller.is_in_game: return
        
        # Khóa nút trợ giúp khi đang chờ
        for btn in [self.btn_5050, self.btn_audit, self.btn_call, self.btn_swap]:
            btn.config(state=tk.DISABLED)
            
        res = self.controller.network.send_request({"type": "CHECK_GAME_STATUS"})
        
        status = res.get("game_status")
        if status == "FINISHED":
            self.opp_score = res.get("opponent_score", 0)
            you_win = res.get("you_win", False)
            self.show_game_over_pvp(you_win)
        else:
             self.after(2000, self.wait_opponent)

    def use_lifeline(self, lid):
        if lid in self.lifelines_used: return
        
        if messagebox.askyesno("Trợ giúp", "Bạn muốn dùng quyền trợ giúp này?"):
            res = self.controller.network.send_request({
                "type": "USE_LIFELINE",
                "lifeline_id": lid
            })
            
            if res.get("type") == "LIFELINE_RES":
                data = res.get("data")
                if data.get("status") == "OK":
                    self.lifelines_used.append(lid)
                    self.handle_lifeline_effect(lid, data)
                    
                    btn_map = {1: self.btn_5050, 2: self.btn_audit, 3: self.btn_call, 4: self.btn_swap}
                    if lid in btn_map:
                        btn_map[lid].config(state=tk.DISABLED, bg="gray")
                else:
                    messagebox.showwarning("Lỗi", "Không thể sử dụng (Server từ chối).")

    def handle_lifeline_effect(self, lid, data):
        if lid == 1: # 50:50
            removed = data.get("removed_indexes", [])
            for idx in removed:
                self.btn_opts[idx].config(text="", state=tk.DISABLED, bg="#e0e0e0")
                
        elif lid == 2: # Audience
            percents = data.get("percentages", [])
            msg = f"A: {percents[0]}% | B: {percents[1]}%\nC: {percents[2]}% | D: {percents[3]}%"
            messagebox.showinfo("Khán giả bình chọn", msg)
            
        elif lid == 3: # Call
            s_idx = data.get("suggested_index")
            name = data.get("friend_name", "Người thân")
            char_ans = chr(65 + s_idx)
            messagebox.showinfo("Gọi điện thoại", f"{name}: Tôi nghĩ đáp án là {char_ans}")
            
        elif lid == 4: # Swap
            new_q = data.get("new_question")
            new_opts = data.get("new_options")
            self.current_q_id = data.get("new_id")
            
            self.lbl_question.config(text=f"CÂU HỎI MỚI:\n{new_q}")
            for i, opt in enumerate(new_opts):
                self.btn_opts[i].config(text=f"{chr(65+i)}. {opt}", state=tk.NORMAL, bg="white")

    def show_opponent_quit(self, opponent_name):
        """Hiển thị thông báo khi đối thủ đầu hàng"""
        self.stop_timer()
        self.controller.is_in_game = False
        
        # Tạo cửa sổ popup tùy chỉnh
        popup = tk.Toplevel(self)
        popup.title("Chiến thắng!")
        popup.geometry("400x250")
        popup.configure(bg=BG_PRIMARY)
        popup.resizable(False, False)
        
        # Căn giữa màn hình
        popup.transient(self)
        popup.grab_set()
        
        # Nội dung
        tk.Label(popup, text="🏆 CHIẾN THẮNG! 🏆", 
                font=("Segoe UI", 20, "bold"), fg=SUCCESS_COLOR, bg=BG_PRIMARY).pack(pady=20)
        
        tk.Label(popup, text=f"{opponent_name} đã đầu hàng!\n\nBạn đã thắng trận đấu này.", 
                font=("Segoe UI", 14), fg=TEXT_LIGHT, bg=BG_PRIMARY, justify=tk.CENTER).pack(pady=10)
        
        tk.Label(popup, text=f"Điểm của bạn: {self.my_score}", 
                font=("Segoe UI", 12, "bold"), fg="white", bg=BG_PRIMARY).pack(pady=5)
        
        # Nút quay về sảnh
        def return_to_lobby():
            popup.destroy()
            self.quit_game()
        
        create_styled_button(popup, "Quay về sảnh chờ", return_to_lobby, SUCCESS_COLOR, width=20).pack(pady=20)
        
        # Đợi người dùng đóng popup
        popup.wait_window()
    
    def show_game_over_pvp(self, you_win):
        self.stop_timer()
        self.controller.is_in_game = False
        title = "CHIẾN THẮNG!" if you_win else "THUA CUỘC"
        msg = f"{title}\nĐiểm bạn: {self.my_score}\nĐối thủ: {self.opp_score}"
        messagebox.showinfo("Kết quả trận đấu", msg)
        self.quit_game()

    def show_game_over_classic(self):
        self.stop_timer()
        self.controller.is_in_game = False
        messagebox.showinfo("Hoàn thành", f"Chúc mừng!\nBạn đã hoàn thành lượt chơi.\nTổng điểm: {self.my_score}")
        self.quit_game()

    def quit_game(self):
        self.stop_timer()
        self.controller.is_in_game = False
        self.controller.network.send_request({"type": "QUIT_GAME"})
        self.controller.show_frame("LobbyView")
        if hasattr(self.controller.frames["LobbyView"], "refresh_lobby"):
            self.controller.frames["LobbyView"].refresh_lobby()