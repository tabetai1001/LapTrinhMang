import tkinter as tk
from tkinter import messagebox
from core.network import NetworkManager
from core.config import *
from ui.view_auth import AuthView
from ui.view_lobby import LobbyView
from ui.view_game import GameView
from ui.view_history import HistoryView

class ClientApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Ai Là Triệu Phú - Online")
        self.geometry("1000x700")
        self.minsize(900, 650)
        self.configure(bg=BG_PRIMARY)
        
        # --- Core ---
        self.network = NetworkManager()
        self.current_user = None
        self.is_in_game = False
        self.is_polling = False
        
        # --- UI Manager ---
        self.container = tk.Frame(self, bg=BG_PRIMARY)
        self.container.pack(side="top", fill="both", expand=True)
        self.container.grid_rowconfigure(0, weight=1)
        self.container.grid_columnconfigure(0, weight=1)
        
        self.frames = {}
        self.init_views()
        self.show_frame("AuthView")

    def init_views(self):
        # Khởi tạo tất cả các màn hình
        for V in (AuthView, LobbyView, GameView, HistoryView):
            page_name = V.__name__
            frame = V(parent=self.container, controller=self)
            self.frames[page_name] = frame
            frame.grid(row=0, column=0, sticky="nsew")

    def show_frame(self, page_name):
        frame = self.frames[page_name]
        frame.tkraise()
        # Gọi hàm on_show nếu màn hình đó có định nghĩa (để refresh dữ liệu)
        if hasattr(frame, "on_show"):
            frame.on_show()

    def start_polling(self):
        self.is_polling = True
        self.poll_server()

    def poll_server(self):
        if not self.is_polling: return
        
        try:
            # Gửi yêu cầu thăm dò server
            res = self.network.send_request({"type": "POLL"})
            msg_type = res.get("type")
            
            # 1. Có người mời thách đấu
            if msg_type == "RECEIVE_INVITE":
                inviter = res.get("from")
                ans = messagebox.askyesno("Thách đấu", f"{inviter} muốn thách đấu bạn?")
                if ans:
                    self.network.send_request({"type": "ACCEPT_INVITE", "from": inviter})
                else:
                    self.network.send_request({"type": "REJECT_INVITE", "from": inviter})
            
            # 2. Game bắt đầu (Do chấp nhận mời HOẶC bấm Chơi đơn)
            elif msg_type == "GAME_START":
                opponent = res.get("opponent")
                # QUAN TRỌNG: Lấy mode từ server (PVP hoặc CLASSIC)
                mode = res.get("mode", "PVP") 
                
                # Chỉ chuyển cảnh nếu chưa ở trong game
                if not self.is_in_game:
                    print(f"[Main] Starting game: {mode} vs {opponent}")
                    self.frames["GameView"].start_game(opponent, mode=mode)
                    self.show_frame("GameView")
                
            # 3. Cập nhật danh sách Lobby
            elif msg_type == "LOBBY_LIST":
                if hasattr(self.frames["LobbyView"], "refresh_lobby"):
                    self.frames["LobbyView"].refresh_lobby()
            
            # 4. Đối thủ thoát game (đầu hàng)
            elif msg_type == "OPPONENT_QUIT":
                if self.is_in_game:
                    opponent_name = res.get("opponent", "Đối thủ")
                    self.frames["GameView"].show_opponent_quit(opponent_name)
            
            # 5. Tin nhắn chat mới
            elif msg_type == "NEW_CHAT_MESSAGE":
                username = res.get("username")
                message = res.get("message")
                if hasattr(self.frames["LobbyView"], "add_chat_message"):
                    self.frames["LobbyView"].add_chat_message(username, message)
                    
        except Exception as e:
            print(f"Polling error: {e}")
            
        # Lặp lại sau 1 giây
        self.after(1000, self.poll_server)

if __name__ == "__main__":
    app = ClientApp()
    app.mainloop()