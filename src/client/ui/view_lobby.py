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
        
        # Main content area - 2 columns
        content_frame = tk.Frame(self, bg=BG_PRIMARY)
        content_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=10)
        content_frame.columnconfigure(0, weight=1)
        content_frame.columnconfigure(1, weight=1)
        content_frame.rowconfigure(0, weight=1)
        
        # LEFT: Player List Card
        player_card = create_styled_frame(content_frame, CARD_BG)
        player_card.grid(row=0, column=0, sticky="nsew", padx=(0, 10))
        
        tk.Label(player_card, text="Danh sách người chơi", font=("Segoe UI", 14, "bold"), bg=CARD_BG, fg=TEXT_DARK).pack(pady=5)
        
        # Scrollable List
        list_frame = tk.Frame(player_card, bg=CARD_BG)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        self.canvas = tk.Canvas(list_frame, bg="white", highlightthickness=0)
        self.scrollbar = tk.Scrollbar(list_frame, orient="vertical", command=self.canvas.yview)
        self.scrollable_frame = tk.Frame(self.canvas, bg="white")
        
        self.scrollable_frame.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.create_window((0, 0), window=self.scrollable_frame, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        
        self.canvas.pack(side="left", fill="both", expand=True)
        self.scrollbar.pack(side="right", fill="y")
        
        # Refresh button
        create_styled_button(player_card, "Làm mới", self.refresh_lobby, BG_SECONDARY).pack(pady=10)
        
        # RIGHT: Chat Card
        chat_card = create_styled_frame(content_frame, CARD_BG)
        chat_card.grid(row=0, column=1, sticky="nsew", padx=(10, 0))
        
        tk.Label(chat_card, text="💬 Chat Lobby", font=("Segoe UI", 14, "bold"), bg=CARD_BG, fg=TEXT_DARK).pack(pady=5)
        
        # Chat display area
        chat_display_frame = tk.Frame(chat_card, bg=CARD_BG)
        chat_display_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        self.chat_text = tk.Text(chat_display_frame, bg="white", fg=TEXT_DARK, font=("Segoe UI", 10), 
                                 wrap=tk.WORD, state=tk.DISABLED, height=15)
        chat_scroll = tk.Scrollbar(chat_display_frame, command=self.chat_text.yview)
        self.chat_text.config(yscrollcommand=chat_scroll.set)
        
        self.chat_text.pack(side="left", fill="both", expand=True)
        chat_scroll.pack(side="right", fill="y")
        
        # Chat input area
        chat_input_frame = tk.Frame(chat_card, bg=CARD_BG)
        chat_input_frame.pack(fill=tk.X, padx=10, pady=10)
        
        self.chat_entry = tk.Entry(chat_input_frame, font=("Segoe UI", 11), bg="white")
        self.chat_entry.pack(side="left", fill="x", expand=True, padx=(0, 5))
        self.chat_entry.bind("<Return>", lambda e: self.send_chat_message())
        
        create_styled_button(chat_input_frame, "Gửi", self.send_chat_message, ACCENT_COLOR, width=8).pack(side="right")
        
        action_frame = tk.Frame(self, bg=BG_PRIMARY)
        action_frame.pack(pady=10, fill=tk.X, padx=20)
        
        # Configure grid to be responsive
        for i in range(4):
            action_frame.columnconfigure(i, weight=1, uniform="button")
        
        create_styled_button(action_frame, "Thách đấu", self.on_invite, ACCENT_COLOR).grid(row=0, column=0, padx=5, sticky="ew")
        create_styled_button(action_frame, "Chơi Đơn", self.on_play_classic, SUCCESS_COLOR).grid(row=0, column=1, padx=5, sticky="ew")
        create_styled_button(action_frame, "Lịch sử", lambda: self.controller.show_frame("HistoryView"), WARNING_COLOR, TEXT_DARK).grid(row=0, column=2, padx=5, sticky="ew")
        create_styled_button(action_frame, "Đăng xuất", self.on_logout, DANGER_COLOR).grid(row=0, column=3, padx=5, sticky="ew")

    def on_show(self):
        """Được gọi khi chuyển sang màn hình này"""
        self.lbl_welcome.config(text=f"Xin chào, {self.controller.current_user}!")
        self.refresh_lobby()
        self.load_chat_history()

    def refresh_lobby(self):
        res = self.controller.network.send_request({"type": "GET_LOBBY_LIST", "include_offline": True})
        self.update_list(res.get("players", []))

    def update_list(self, players):
        for widget in self.scrollable_frame.winfo_children():
            widget.destroy()
        
        self.list_items = []
        self.player_labels = {}  # Store mapping of name to label widget
        
        for p in players:
            name = p.get("name", p.get("user"))
            status = p.get("status", "FREE")
            
            # Color coding
            bg = "#c8e6c9" if status == "FREE" else "#ffcdd2" if status == "IN_GAME" else "#e0e0e0"
            text = f"{'[FREE]' if status=='FREE' else ''} {name} - {status}"
            
            item = tk.Label(self.scrollable_frame, text=text, bg=bg, fg=TEXT_DARK, 
                            font=("Segoe UI", 11), anchor="w", padx=10, pady=5, relief=tk.RIDGE, borderwidth=2)
            item.pack(fill=tk.X, pady=1)
            
            # Click event
            item.bind("<Button-1>", lambda e, n=name, lbl=item: self.select_player(n, lbl))
            self.list_items.append(item)
            self.player_labels[name] = item

    def select_player(self, name, label):
        self.selected_player = name
        
        # Reset all items to normal style
        for item in self.list_items:
            item.config(relief=tk.RIDGE, borderwidth=2)
        
        # Highlight selected player with bold border and different relief
        label.config(relief=tk.SOLID, borderwidth=4)

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
    
    def send_chat_message(self):
        """Gửi tin nhắn chat"""
        message = self.chat_entry.get().strip()
        if not message:
            return
        
        # Gửi tin nhắn lên server
        res = self.controller.network.send_request({
            "type": "SEND_CHAT",
            "message": message
        })
        
        if res.get("type") == "CHAT_SUCCESS":
            self.chat_entry.delete(0, tk.END)
        else:
            messagebox.showerror("Lỗi", "Không thể gửi tin nhắn")
    
    def add_chat_message(self, username, message, is_system=False):
        """Thêm tin nhắn vào khung chat"""
        self.chat_text.config(state=tk.NORMAL)
        
        if is_system:
            # Tin nhắn hệ thống (màu xám, italic)
            self.chat_text.insert(tk.END, f"[Hệ thống] {message}\n", "system")
            self.chat_text.tag_config("system", foreground="gray", font=("Segoe UI", 9, "italic"))
        else:
            # Tin nhắn người dùng
            is_me = (username == self.controller.current_user)
            color = ACCENT_COLOR if is_me else TEXT_DARK
            tag = "me" if is_me else "other"
            
            self.chat_text.insert(tk.END, f"{username}: ", tag + "_name")
            self.chat_text.insert(tk.END, f"{message}\n", tag)
            
            self.chat_text.tag_config(tag + "_name", foreground=color, font=("Segoe UI", 10, "bold"))
            self.chat_text.tag_config(tag, foreground=TEXT_DARK)
        
        self.chat_text.config(state=tk.DISABLED)
        self.chat_text.see(tk.END)  # Auto-scroll xuống dưới
    
    def load_chat_history(self):
        """Tải lịch sử chat từ server"""
        # Clear chat text trước khi load để tránh duplicate
        self.chat_text.config(state=tk.NORMAL)
        self.chat_text.delete(1.0, tk.END)
        self.chat_text.config(state=tk.DISABLED)
        
        res = self.controller.network.send_request({"type": "GET_CHAT_HISTORY"})
        
        if res.get("type") == "CHAT_HISTORY":
            messages = res.get("messages", [])
            for msg in messages:
                self.add_chat_message(msg.get("username"), msg.get("message"))