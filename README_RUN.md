# HƯỚNG DẪN CHẠY DỰ ÁN AI LÀ TRIỆU PHÚ

## 🚀 SETUP NHANH (Khuyến nghị)

**Server chạy trên WSL/Linux + Client chạy trên Windows**

---

## BƯỚC 1: BUILD VÀ CHẠY SERVER (WSL)

Mở **WSL terminal** (Ubuntu):

```bash
# Di chuyển vào thư mục dự án
cd /mnt/d/20251/mili

# Build server
make clean
make server

# Chạy server
./bin/server
```

✅ Server sẽ hiển thị: `[Server] Dang chay tai port 5555...`

**Giữ terminal này mở!**

---

## BƯỚC 2: BUILD CLIENT DLL (Windows)

Mở **MinGW/MSYS terminal** hoặc **Git Bash**:

```bash
# Di chuyển vào thư mục dự án
cd /d/20251/mili

# Build client DLL
gcc -shared -o bin/client_network.dll src/client/native/client_network_windows.c src/common/cJSON.c -lws2_32 -I./src/common

# Kiểm tra
ls -lh bin/client_network.dll
```

✅ Bạn sẽ thấy file `bin/client_network.dll` được tạo

---

## BƯỚC 3: CHẠY CLIENT (Windows)

Mở **CMD** hoặc **PowerShell**:

```cmd
cd d:\20251\mili
python src\client\main.py
```

✅ Giao diện game sẽ hiện ra!

---

## 📋 YÊU CẦU HỆ THỐNG

### WSL/Linux (Server):
```bash
sudo apt update
sudo apt install -y build-essential make
```

### Windows (Client):
- Python 3.x (đã cài Tkinter)
- MinGW/MSYS hoặc Git Bash (để build DLL)

---

## 🎮 SỬ DỤNG

1. **Đăng ký tài khoản:** Nhập username và password → Đăng ký
2. **Đăng nhập:** Dùng tài khoản vừa tạo
3. **Chơi đơn (Classic):** Bấm "Chơi đơn" → Trả lời câu hỏi
4. **Chơi PvP:** Chọn người chơi trong lobby → Mời thách đấu

---

## 🐛 XỬ LÝ LỖI

### Server không chạy:
```bash
# Kiểm tra port đã bị chiếm chưa
ss -tulpn | grep 5555

# Kill process nếu cần
pkill -f server
```

### Client không kết nối:
- Kiểm tra server đang chạy trong WSL
- Đảm bảo file `bin/client_network.dll` tồn tại
- Thử kết nối: `telnet localhost 5555`

### Lỗi "cannot find client_network.dll":
- Chạy lại build DLL (Bước 2)
- Chạy Python từ thư mục gốc: `d:\20251\mili`

---

## 📁 CẤU TRÚC THƯ MỤC

```
d:\20251\mili\
├── bin/
│   ├── server              # Linux executable (WSL)
│   ├── client_network.dll  # Windows DLL
│   └── client_network.so   # Linux SO (không dùng cho Windows client)
├── data/
│   ├── accounts.json       # Tài khoản người chơi
│   ├── questions.json      # Ngân hàng câu hỏi
│   └── history.json        # Lịch sử trận đấu
├── src/
│   ├── server/             # Server code (C)
│   ├── client/             # Client code (Python + C)
│   └── common/             # Shared code
└── README_RUN.md           # File này
```

---

## ⚡ QUY TRÌNH LÀM VIỆC HÀNG NGÀY

### Khởi động:
1. Mở WSL terminal → chạy `./bin/server`
2. Mở CMD/PowerShell → chạy `python src\client\main.py`

### Sửa code server:
```bash
# Trong WSL
make clean && make server
./bin/server
```

### Sửa code client Python:
```cmd
REM Chỉ cần chạy lại
python src\client\main.py
```

### Sửa code client C (native):
```bash
# Trong MinGW/Git Bash
gcc -shared -o bin/client_network.dll src/client/native/client_network_windows.c src/common/cJSON.c -lws2_32 -I./src/common
```

---

## 📚 TÀI LIỆU THÊM

- [README_WSL.md](README_WSL.md) - Hướng dẫn chi tiết về WSL
- [SETUP_HYBRID.md](SETUP_HYBRID.md) - Chi tiết setup hybrid
- [MIGRATION_SUMMARY.md](MIGRATION_SUMMARY.md) - Tóm tắt thay đổi Linux

---

## 🎯 KIỂM TRA HOẠT ĐỘNG

Sau khi setup:

- [x] Server WSL hiển thị: `[Server] Dang chay tai port 5555...`
- [x] Client Windows kết nối thành công
- [x] Đăng ký/Đăng nhập OK
- [x] Lobby list hiển thị người chơi online
- [x] Chơi game Classic hoạt động
- [x] Chơi game PvP hoạt động
- [x] Chat room hoạt động

**Tất cả OK → Chúc mừng bạn đã setup thành công!** 🎉

---

## 📞 HỖ TRỢ

**Port:** 5555  
**IP:** localhost / 127.0.0.1  
**Protocol:** TCP  

**Lưu ý:** Server WSL và Client Windows tự động kết nối qua localhost, không cần cấu hình thêm!
