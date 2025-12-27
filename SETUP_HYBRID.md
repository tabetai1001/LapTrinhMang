# Setup Hybrid: Server WSL + Client Windows

Hướng dẫn chạy Server trên WSL/Linux và Client trên Windows native.

## ✅ Ưu điểm của phương án này:

- Server chạy trên WSL (Linux native performance)
- Client chạy trên Windows (GUI hoạt động tốt nhất)
- Không cần X Server
- Kết nối qua `localhost` rất nhanh

---

## 🚀 SETUP NHANH

### Bước 1: Build Server trên WSL

```bash
# Mở WSL terminal
cd /mnt/d/20251/mili

# Build server
make clean
make server

# Chạy server
./bin/server
```

Server sẽ lắng nghe trên `0.0.0.0:5555` (có thể truy cập từ Windows).

### Bước 2: Build Client DLL trên Windows

**Cách 1: Sử dụng script tự động (khuyến nghị)**

```cmd
REM Mở CMD hoặc PowerShell trên Windows
cd d:\20251\mili
build_client_windows.bat
```

**Cách 2: Build thủ công**

```cmd
REM Cần có GCC (MinGW-w64)
gcc -shared -o bin\client_network.dll src\client\native\client_network_windows.c src\common\cJSON.c -lws2_32 -I.\src\common
```

### Bước 3: Chạy Client trên Windows

```cmd
cd d:\20251\mili
python src\client\main.py
```

Client sẽ tự động kết nối đến `127.0.0.1:5555`.

---

## 📋 YÊU CẦU HỆ THỐNG

### WSL (Server):
- Ubuntu/Debian hoặc distro tương tự
- GCC: `sudo apt install build-essential`
- Make: `sudo apt install make`

### Windows (Client):
- Python 3.x: https://www.python.org/downloads/
- Tkinter (thường đi kèm Python)
- GCC cho Windows (nếu cần build DLL):
  - MinGW-w64: https://www.mingw-w64.org/
  - Hoặc: https://github.com/niXman/mingw-builds-binaries/releases

---

## 🔧 KẾT NỐI

### Từ Windows Client → WSL Server:

Client kết nối đến: `127.0.0.1:5555` hoặc `localhost:5555`

WSL2 tự động bridge network với Windows, nên localhost hoạt động ngay.

### Kiểm tra server đang chạy:

**Trong WSL:**
```bash
ps aux | grep server
ss -tulpn | grep 5555
```

**Từ Windows:**
```cmd
netstat -an | findstr 5555
```

---

## 🐛 TROUBLESHOOTING

### Client không kết nối được:

1. **Kiểm tra server đang chạy:**
   ```bash
   # Trong WSL
   ./bin/server
   ```

2. **Kiểm tra Windows Firewall:**
   - Mở Windows Defender Firewall
   - Allow WSL qua firewall

3. **Test kết nối từ Windows:**
   ```cmd
   telnet localhost 5555
   ```

### DLL không load được:

1. **Kiểm tra file tồn tại:**
   ```cmd
   dir bin\client_network.dll
   ```

2. **Kiểm tra dependencies:**
   - Đảm bảo có `ws2_32.dll` (Windows có sẵn)

3. **Rebuild:**
   ```cmd
   build_client_windows.bat
   ```

### Python không tìm thấy DLL:

Đảm bảo chạy Python từ thư mục gốc dự án:
```cmd
cd d:\20251\mili
python src\client\main.py
```

---

## 📁 CẤU TRÚC FILE

```
d:\20251\mili\
├── bin/
│   ├── server (WSL - Linux executable)
│   ├── client_network.so (WSL - không dùng cho Windows)
│   └── client_network.dll (Windows - client sử dụng)
├── src/
│   ├── server/ (code cho WSL)
│   ├── client/
│   │   ├── main.py (chạy trên Windows)
│   │   └── native/
│   │       ├── client_network.c (Linux version)
│   │       └── client_network_windows.c (Windows version)
│   └── common/
├── Makefile (cho WSL)
└── build_client_windows.bat (cho Windows)
```

---

## ⚡ QUY TRÌNH LÀM VIỆC HÀNG NGÀY

### 1. Khởi động server (WSL):
```bash
cd /mnt/d/20251/mili
./bin/server
```

### 2. Chạy client (Windows):
```cmd
cd d:\20251\mili
python src\client\main.py
```

### 3. Khi sửa code server:
```bash
# Trong WSL
make clean
make server
./bin/server
```

### 4. Khi sửa code client:
```cmd
REM Trong Windows - nếu sửa C code
build_client_windows.bat

REM Nếu chỉ sửa Python
python src\client\main.py
```

---

## 🎯 LƯU Ý QUAN TRỌNG

1. **Server (WSL):**
   - Build trên WSL/Linux
   - Binary: `bin/server`
   - Không cần Winsock

2. **Client (Windows):**
   - Build trên Windows
   - Binary: `bin/client_network.dll`
   - Cần Winsock (ws2_32)

3. **Không dùng chung binary** giữa WSL và Windows

4. **Network:** localhost/127.0.0.1 hoạt động tự động

5. **File data:** Chia sẻ qua `/mnt/d/` (WSL) và `d:\` (Windows)

---

## ✅ KIỂM TRA HOẠT ĐỘNG

Sau khi setup xong:

1. ✅ Server WSL chạy, log ra: `[Server] Dang chay tai port 5555...`
2. ✅ Client Windows kết nối thành công
3. ✅ Có thể đăng ký/đăng nhập
4. ✅ Lobby list hiển thị
5. ✅ Chơi game bình thường

**Nếu tất cả OK → Setup thành công!** 🎉

---

## 📞 HỖ TRỢ

- Đọc: [README_WSL.md](README_WSL.md) - Hướng dẫn WSL chi tiết
- Đọc: [MIGRATION_SUMMARY.md](MIGRATION_SUMMARY.md) - Tóm tắt thay đổi

**Status:** ✅ Recommended Setup (Best of both worlds)
