# src/client/ui/view_auth.py
import tkinter as tk
from tkinter import messagebox
from ui.widgets import *
from core.config import *

class AuthView(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent, bg=BG_PRIMARY)
        self.controller = controller
        
        self.frame_connect = None
        self.frame_login = None
        
        self.setup_connect_ui()
        self.setup_login_ui()
        
        self.show_connect() # Mặc định hiện connect

    def setup_connect_ui(self):
        self.frame_connect = create_styled_frame(self, BG_PRIMARY)
        
        tk.Label(self.frame_connect, text="AI LÀ TRIỆU PHÚ", font=("Segoe UI", 28, "bold"), fg=TEXT_LIGHT, bg=BG_PRIMARY).pack(pady=30)
        tk.Label(self.frame_connect, text="Kết nối đến máy chủ", font=("Segoe UI", 14), fg=WARNING_COLOR, bg=BG_PRIMARY).pack(pady=10)
        
        card = create_styled_frame(self.frame_connect, CARD_BG)
        card.pack(pady=20, padx=100, fill=tk.X)
        
        tk.Label(card, text="IP:", bg=CARD_BG, fg=TEXT_DARK, font=("Segoe UI", 11, "bold")).pack(pady=5)
        self.entry_ip = create_styled_entry(card)
        self.entry_ip.insert(0, DEFAULT_IP)
        self.entry_ip.pack(pady=5)
        
        tk.Label(card, text="Port:", bg=CARD_BG, fg=TEXT_DARK, font=("Segoe UI", 11, "bold")).pack(pady=5)
        self.entry_port = create_styled_entry(card)
        self.entry_port.insert(0, str(DEFAULT_PORT))
        self.entry_port.pack(pady=5)
        
        create_styled_button(card, "Kết nối", self.on_connect, SUCCESS_COLOR).pack(pady=20)

    def setup_login_ui(self):
        self.frame_login = create_styled_frame(self, BG_PRIMARY)
        
        tk.Label(self.frame_login, text="ĐĂNG NHẬP", font=("Segoe UI", 28, "bold"), fg=TEXT_LIGHT, bg=BG_PRIMARY).pack(pady=30)
        
        card = create_styled_frame(self.frame_login, CARD_BG)
        card.pack(pady=20, padx=100, fill=tk.X)
        
        tk.Label(card, text="Username:", bg=CARD_BG, fg=TEXT_DARK, font=("Segoe UI", 11, "bold")).pack(pady=5)
        self.entry_user = create_styled_entry(card)
        self.entry_user.pack(pady=5)
        
        tk.Label(card, text="Password:", bg=CARD_BG, fg=TEXT_DARK, font=("Segoe UI", 11, "bold")).pack(pady=5)
        self.entry_pass = create_styled_entry(card, show="*")
        self.entry_pass.pack(pady=5)
        
        btn_frame = tk.Frame(card, bg=CARD_BG)
        btn_frame.pack(pady=20)
        
        create_styled_button(btn_frame, "Đăng nhập", self.on_login, SUCCESS_COLOR, width=12).pack(side=tk.LEFT, padx=5)
        create_styled_button(btn_frame, "Đăng ký", self.on_register, ACCENT_COLOR, width=12).pack(side=tk.LEFT, padx=5)

    def show_connect(self):
        if self.frame_login: self.frame_login.pack_forget()
        self.frame_connect.pack(fill=tk.BOTH, expand=True)

    def show_login(self):
        self.frame_connect.pack_forget()
        self.frame_login.pack(fill=tk.BOTH, expand=True)

    def on_connect(self):
        ip = self.entry_ip.get()
        try:
            port = int(self.entry_port.get())
            if self.controller.network.connect(ip, port):
                self.show_login()
            else:
                messagebox.showerror("Lỗi", "Không thể kết nối Server!")
        except ValueError:
            messagebox.showerror("Lỗi", "Port phải là số!")

    def on_login(self):
        u = self.entry_user.get()
        p = self.entry_pass.get()
        res = self.controller.network.send_request({"type": "LOGIN", "user": u, "pass": p})
        
        if res.get("type") == "LOGIN_SUCCESS":
            self.controller.current_user = u
            self.controller.show_frame("LobbyView")
            self.controller.start_polling() # Bắt đầu polling
        else:
            messagebox.showerror("Lỗi", res.get("message", "Đăng nhập thất bại"))

    def on_register(self):
        u = self.entry_user.get()
        p = self.entry_pass.get()
        res = self.controller.network.send_request({"type": "REGISTER", "user": u, "pass": p})
        if res.get("type") == "REGISTER_SUCCESS":
            messagebox.showinfo("Thành công", "Đăng ký thành công!")
        else:
            messagebox.showerror("Lỗi", res.get("message"))