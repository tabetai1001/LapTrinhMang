# src/client/ui/view_lobby.py
import tkinter as tk
from tkinter import messagebox
from ui.widgets import *
from core.config import *

class LobbyView(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent, bg=BG_PRIMARY)
        self.controller = controller
        self.list_items = []
        self.selected_player = None
        
        self.setup_ui()

    def setup_ui(self):
        # Header
        self.header_frame = tk.Frame(self, bg=BG_PRIMARY)
        self.header_frame.pack(fill=tk.X, pady=10)
        self.lbl_welcome = tk.Label(self.header_frame, text="...", font=("Segoe UI", 18, "bold"), fg=TEXT_LIGHT, bg=BG_PRIMARY)
        self.lbl_welcome.pack()
        
        # Player List Card
        card = create_styled_frame(self, CARD_BG)
        card.pack(pady=20, padx=50, fill=tk.BOTH, expand=True)
        
        tk.Label(card, text="Danh sách người chơi", font=("Segoe UI", 14, "bold"), bg=CARD_BG, fg=TEXT_DARK).pack(pady=5)
        
        # Scrollable List
        list_frame = tk.Frame(card, bg=CARD_BG)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        self.canvas = tk.Canvas(list_frame, bg="white", highlightthickness=0)
        self.scrollbar = tk.Scrollbar(list_frame, orient="vertical", command=self.canvas.yview)
        self.scrollable_frame = tk.Frame(self.canvas, bg="white")
        
        self.scrollable_frame.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.create_window((0, 0), window=self.scrollable_frame, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        
        self.canvas.pack(side="left", fill="both", expand=True)
        self.scrollbar.pack(side="right", fill="y")
        
        # Buttons
        create_styled_button(card, "Làm mới", self.refresh_lobby, BG_SECONDARY).pack(pady=10)
        
        action_frame = tk.Frame(self, bg=BG_PRIMARY)
        action_frame.pack(pady=20)
        create_styled_button(action_frame, "Thách đấu", self.on_invite, ACCENT_COLOR).pack(side=tk.LEFT, padx=10)
        create_styled_button(action_frame, "Chơi Đơn", self.on_play_classic, SUCCESS_COLOR).pack(side=tk.LEFT, padx=10)
        create_styled_button(action_frame, "Lịch sử", lambda: self.controller.show_frame("HistoryView"), WARNING_COLOR, TEXT_DARK).pack(side=tk.LEFT, padx=10)
        create_styled_button(action_frame, "Đăng xuất", self.on_logout, DANGER_COLOR).pack(side=tk.LEFT, padx=10)

    def on_show(self):
        """Được gọi khi chuyển sang màn hình này"""
        self.lbl_welcome.config(text=f"Xin chào, {self.controller.current_user}!")
        self.refresh_lobby()

    def refresh_lobby(self):
        res = self.controller.network.send_request({"type": "GET_LOBBY_LIST", "include_offline": True})
        self.update_list(res.get("players", []))

    def update_list(self, players):
        for widget in self.scrollable_frame.winfo_children():
            widget.destroy()
        
        self.list_items = []
        for p in players:
            name = p.get("name", p.get("user"))
            status = p.get("status", "FREE")
            
            # Color coding
            bg = "#c8e6c9" if status == "FREE" else "#ffcdd2" if status == "IN_GAME" else "#e0e0e0"
            text = f"{'[FREE]' if status=='FREE' else ''} {name} - {status}"
            
            item = tk.Label(self.scrollable_frame, text=text, bg=bg, fg=TEXT_DARK, 
                            font=("Segoe UI", 11), anchor="w", padx=10, pady=5, relief=tk.RIDGE)
            item.pack(fill=tk.X, pady=1)
            
            # Click event
            item.bind("<Button-1>", lambda e, n=name: self.select_player(n))
            self.list_items.append(item)

    def select_player(self, name):
        self.selected_player = name
        messagebox.showinfo("Chọn", f"Đã chọn: {name}")

    def on_invite(self):
        if not self.selected_player:
            messagebox.showwarning("Lỗi", "Chưa chọn người chơi!")
            return
        if self.selected_player == self.controller.current_user:
            messagebox.showwarning("Lỗi", "Không thể tự thách đấu!")
            return
            
        # Logic gửi mời đơn giản (mặc định 5 câu)
        res = self.controller.network.send_request({
            "type": "INVITE_PLAYER", 
            "target": self.selected_player, 
            "num_questions": 5
        })
        if res.get("type") == "INVITE_SENT_SUCCESS":
            messagebox.showinfo("Đã gửi", "Đang chờ đối thủ...")
        else:
            messagebox.showerror("Lỗi", res.get("message"))
    
    def on_play_classic(self):
        res = self.controller.network.send_request({"type": "START_CLASSIC"})
        if res.get("type") == "GAME_START":
            print("[Lobby] Request Classic Mode sent. Waiting for server...")
        else:
            messagebox.showerror("Lỗi", "Không thể bắt đầu game!")

    def on_logout(self):
        self.controller.network.send_request({"type": "LOGOUT"})
        self.controller.current_user = None
        self.controller.is_polling = False
        self.controller.show_frame("AuthView")