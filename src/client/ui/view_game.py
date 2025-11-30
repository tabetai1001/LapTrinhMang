# src/client/ui/view_game.py
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
        self.opponent = ""
        self.question_start_time = 0
        self.timer_id = None
        self.current_q_id = 0
        self.my_score = 0
        self.opp_score = 0
        
        self.setup_ui()

    def setup_ui(self):
        self.header = tk.Frame(self, bg=BG_SECONDARY)
        self.header.pack(fill=tk.X, pady=10)
        
        self.lbl_info = tk.Label(self.header, text="", font=("Segoe UI", 14, "bold"), fg="white", bg=BG_SECONDARY)
        self.lbl_info.pack()
        
        self.lbl_timer = tk.Label(self.header, text="", font=("Segoe UI", 12, "bold"), fg=WARNING_COLOR, bg=BG_SECONDARY)
        self.lbl_timer.pack()
        
        self.q_card = create_styled_frame(self, CARD_BG)
        self.q_card.pack(pady=20, padx=30, fill=tk.BOTH)
        
        self.lbl_question = tk.Label(self.q_card, text="", font=("Segoe UI", 16), wraplength=700, bg=CARD_BG)
        self.lbl_question.pack(pady=20)
        
        self.ans_frame = tk.Frame(self, bg=BG_PRIMARY)
        self.ans_frame.pack(pady=10)
        
        self.btn_opts = []
        for i in range(4):
            btn = tk.Button(self.ans_frame, text="", font=("Segoe UI", 12), width=30,
                           command=lambda idx=i: self.submit_answer(idx))
            btn.grid(row=i//2, column=i%2, padx=10, pady=10)
            self.btn_opts.append(btn)
            
        self.lbl_result = tk.Label(self, text="", font=("Segoe UI", 14, "bold"), bg=BG_PRIMARY, fg=SUCCESS_COLOR)
        self.lbl_result.pack()

        create_styled_button(self, "Thoát Game", self.quit_game, DANGER_COLOR).pack(pady=10)

    def start_game(self, opponent_name):
        self.opponent = opponent_name
        self.my_score = 0
        self.opp_score = 0
        self.controller.is_in_game = True
        self.request_question()

    def request_question(self):
        # Reset UI
        self.lbl_result.config(text="")
        for btn in self.btn_opts:
            btn.config(state=tk.NORMAL, bg="white")
            
        res = self.controller.network.send_request({"type": "REQUEST_QUESTION"})
        
        if res.get("type") == "QUESTION":
            self.display_question(res)
        elif res.get("type") == "NO_MORE_QUESTIONS":
            messagebox.showinfo("Xong", "Đã hết câu hỏi! Đợi kết quả...")
            self.quit_game()

    def display_question(self, data):
        self.current_q_id = data["question_id"]
        self.lbl_question.config(text=f"Câu {data['question_number']}: {data['question']}")
        self.lbl_info.config(text=f"Bạn: {self.my_score} | {self.opponent}: {self.opp_score}")
        
        opts = data["options"]
        for i, opt in enumerate(opts):
            self.btn_opts[i].config(text=f"{chr(65+i)}. {opt}")
            
        self.question_start_time = time.time()
        self.run_timer(15)

    def run_timer(self, seconds):
        if not self.controller.is_in_game: return
        self.lbl_timer.config(text=f"Thời gian: {seconds}s")
        if seconds > 0:
            self.timer_id = self.after(1000, lambda: self.run_timer(seconds-1))
        else:
            self.submit_answer(-1) # Timeout

    def submit_answer(self, idx):
        if self.timer_id: self.after_cancel(self.timer_id)
        
        time_taken = time.time() - self.question_start_time
        
        for btn in self.btn_opts: btn.config(state=tk.DISABLED)
        
        res = self.controller.network.send_request({
            "type": "SUBMIT_ANSWER",
            "question_id": self.current_q_id,
            "answer_index": idx,
            "time_taken": time_taken
        })
        
        if res.get("type") == "ANSWER_RESULT":
            self.show_result(res, idx)

    def show_result(self, res, my_ans):
        correct = res["correct_answer"]
        self.my_score = res["your_total_score"]
        self.opp_score = res["opponent_score"]
        
        if my_ans == correct:
            self.lbl_result.config(text="CHÍNH XÁC!", fg=SUCCESS_COLOR)
            if my_ans != -1: self.btn_opts[my_ans].config(bg=SUCCESS_COLOR)
        else:
            self.lbl_result.config(text="SAI RỒI!", fg=DANGER_COLOR)
            if my_ans != -1: self.btn_opts[my_ans].config(bg=DANGER_COLOR)
            self.btn_opts[correct].config(bg=SUCCESS_COLOR)
            
        status = res.get("game_status")
        if status == "FINISHED":
            self.after(2000, lambda: messagebox.showinfo("Kết quả", f"Game kết thúc!\nBạn: {self.my_score} - {self.opponent}: {self.opp_score}"))
            self.after(2000, self.quit_game)
        elif status == "WAITING_OPPONENT":
            self.lbl_result.config(text="Đợi đối thủ...", fg="yellow")
            self.wait_opponent()
        else:
            self.after(1500, self.request_question)

    def wait_opponent(self):
        # Polling riêng cho game status
        if not self.controller.is_in_game: return
        res = self.controller.network.send_request({"type": "CHECK_GAME_STATUS"})
        if res.get("game_status") == "FINISHED":
             self.quit_game()
        else:
            self.after(2000, self.wait_opponent)

    def quit_game(self):
        if self.timer_id: self.after_cancel(self.timer_id)
        self.controller.is_in_game = False
        self.controller.network.send_request({"type": "QUIT_GAME"})
        self.controller.show_frame("LobbyView")