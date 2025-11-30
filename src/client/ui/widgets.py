# src/client/ui/widgets.py
import tkinter as tk
from core.config import *

def create_styled_button(parent, text, command, bg_color=BG_PRIMARY, fg_color=TEXT_LIGHT, width=20):
    btn = tk.Button(parent, text=text, command=command, 
                   bg=bg_color, fg=fg_color, 
                   font=("Segoe UI", 11, "bold"),
                   relief=tk.FLAT, bd=0,
                   padx=20, pady=10,
                   width=width,
                   cursor="hand2")
    
    def on_enter(e): btn['bg'] = lighten_color(bg_color)
    def on_leave(e): btn['bg'] = bg_color
    
    btn.bind("<Enter>", on_enter)
    btn.bind("<Leave>", on_leave)
    return btn

def create_styled_frame(parent, bg_color=CARD_BG):
    return tk.Frame(parent, bg=bg_color, relief=tk.FLAT, bd=0)

def create_styled_entry(parent, width=30, show=None):
    return tk.Entry(parent, width=width, show=show,
                    font=("Segoe UI", 11),
                    relief=tk.FLAT, bd=2,
                    bg=TEXT_LIGHT, fg=TEXT_DARK)

def lighten_color(hex_color):
    """Làm sáng màu cho hiệu ứng hover"""
    hex_color = hex_color.lstrip('#')
    r, g, b = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
    r = min(255, int(r * 1.2))
    g = min(255, int(g * 1.2))
    b = min(255, int(b * 1.2))
    return f'#{r:02x}{g:02x}{b:02x}'