# src/client/ui/view_history.py
import tkinter as tk
from ui.widgets import *
from core.config import * # Import BG_SECONDARY từ đây
import datetime

class HistoryView(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent, bg=BG_PRIMARY)
        self.controller = controller
        
        tk.Label(self, text="LỊCH SỬ ĐẤU", font=("Segoe UI", 24, "bold"), fg=TEXT_LIGHT, bg=BG_PRIMARY).pack(pady=20)
        
        self.list_frame = tk.Frame(self, bg=BG_PRIMARY)
        self.list_frame.pack(fill=tk.BOTH, expand=True, padx=50)
        
        self.canvas = tk.Canvas(self.list_frame, bg=BG_PRIMARY, highlightthickness=0)
        self.scrollbar = tk.Scrollbar(self.list_frame, orient="vertical", command=self.canvas.yview)
        self.scroll_content = tk.Frame(self.canvas, bg=BG_PRIMARY)
        
        self.scroll_content.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.create_window((0, 0), window=self.scroll_content, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        
        self.canvas.pack(side="left", fill="both", expand=True)
        self.scrollbar.pack(side="right", fill="y")
        
        # --- SỬA LỖI Ở DÒNG DƯỚI ĐÂY ---
        create_styled_button(self, "Quay lại", lambda: controller.show_frame("LobbyView"), BG_SECONDARY).pack(pady=20)

    def on_show(self):
        for w in self.scroll_content.winfo_children(): w.destroy()
        
        res = self.controller.network.send_request({"type": "GET_HISTORY"})
        history = res.get("history", [])
        
        if not history:
            tk.Label(self.scroll_content, text="Chưa có lịch sử!", bg=BG_PRIMARY, fg="gray").pack()
            return
            
        for match in reversed(history):
            bg = "#e8f5e9" if match["result"] == "WIN" else "#ffebee"
            card = tk.Frame(self.scroll_content, bg=bg, pady=10, padx=10)
            card.pack(fill=tk.X, pady=5, padx=5)
            
            result_text = "THẮNG" if match["result"] == "WIN" else "THUA"
            tk.Label(card, text=f"{result_text} vs {match.get('player2')}", font=("Segoe UI", 12, "bold"), bg=bg).pack(anchor="w")
            tk.Label(card, text=f"Tỉ số: {match.get('score1')} - {match.get('score2')}", bg=bg).pack(anchor="w")