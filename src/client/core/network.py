# src/client/core/network.py
import json
import os
import sys
from ctypes import *
import tkinter.messagebox as messagebox

class NetworkManager:
    def __init__(self):
        self.lib = self._load_dll()

    def _load_dll(self):
        """Tìm và load thư viện DLL từ thư mục bin"""
        # Logic tìm file DLL thông minh
        possible_paths = [
            os.path.join("bin", "client_network.dll"), # Chạy từ root
            os.path.join("..", "..", "bin", "client_network.dll"), # Chạy từ src/client
            os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "bin", "client_network.dll"),
            "./client_network.dll"
        ]

        dll_path = None
        for path in possible_paths:
            if os.path.exists(path):
                dll_path = path
                break
        
        if dll_path is None:
            messagebox.showerror("Lỗi nghiêm trọng", "Không tìm thấy 'client_network.dll'!\nHãy kiểm tra thư mục 'bin'.")
            sys.exit(1)

        try:
            lib = CDLL(dll_path)
            lib.connect_to_server.argtypes = [c_char_p, c_int]
            lib.connect_to_server.restype = c_int
            lib.send_request_and_wait.argtypes = [c_char_p]
            lib.send_request_and_wait.restype = c_char_p
            return lib
        except Exception as e:
            messagebox.showerror("Lỗi DLL", f"Không thể load thư viện: {e}")
            sys.exit(1)

    def connect(self, ip, port):
        return self.lib.connect_to_server(ip.encode('utf-8'), port)

    def send_request(self, data):
        """Gửi JSON và nhận phản hồi JSON"""
        try:
            json_str = json.dumps(data)
            res_ptr = self.lib.send_request_and_wait(json_str.encode('utf-8'))
            return json.loads(res_ptr.decode('utf-8'))
        except Exception as e:
            print(f"Network Error: {e}")
            return {}