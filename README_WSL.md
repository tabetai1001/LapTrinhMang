# Hướng dẫn chạy dự án trên WSL (Windows Subsystem for Linux)

## 1. Cài đặt WSL (nếu chưa có)

Mở PowerShell với quyền Administrator và chạy:

```powershell
wsl --install
```

Sau khi cài đặt xong, khởi động lại máy tính.

## 2. Cài đặt các công cụ cần thiết trong WSL

Mở terminal WSL (Ubuntu) và chạy:

```bash
# Cập nhật package manager
sudo apt update && sudo apt upgrade -y

# Cài đặt GCC, Make và các thư viện cần thiết
sudo apt install -y build-essential gcc make

# Cài đặt Python 3 và Tkinter (cho giao diện)
sudo apt install -y python3 python3-pip python3-tk

# Kiểm tra cài đặt
gcc --version
make --version
python3 --version
```

## 3. Truy cập dự án từ WSL

Có 2 cách:

### Cách 1: Truy cập từ ổ đĩa Windows
```bash
cd /mnt/d/20251/mili
```

### Cách 2: Copy dự án vào home directory của WSL (khuyến nghị cho hiệu suất tốt hơn)
```bash
# Từ Windows, copy thư mục vào WSL
# Trong terminal WSL:
cp -r /mnt/d/20251/mili ~/mili
cd ~/mili
```

## 4. Build dự án

```bash
# Compile server và client library
make clean
make all

# Hoặc từng phần:
make server      # Build server
make client_lib  # Build client native library
```

Sau khi build thành công, bạn sẽ có:
- `bin/server` - Server executable
- `bin/client_network.so` - Client native library

## 5. Chạy Server

```bash
# Chạy server
make run

# Hoặc trực tiếp:
./bin/server
```

Server sẽ lắng nghe trên cổng 5555.

## 6. Chạy Client

### Để chạy GUI client trong WSL, cần cấu hình X Server:

#### Windows 10/11:

1. **Cài đặt VcXsrv hoặc X410** (Windows X Server)
   - Tải VcXsrv: https://sourceforge.net/projects/vcxsrv/
   - Hoặc X410 từ Microsoft Store

2. **Khởi động VcXsrv** với cấu hình:
   - Display number: 0
   - Multiple windows
   - Disable access control

3. **Cấu hình DISPLAY trong WSL:**
   ```bash
   # Thêm vào ~/.bashrc hoặc ~/.zshrc
   export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
   
   # Hoặc cho WSL2:
   export DISPLAY=$(ip route list default | awk '{print $3}'):0.0
   
   # Apply changes
   source ~/.bashrc
   ```

4. **Chạy client:**
   ```bash
   cd ~/mili
   python3 src/client/main.py
   ```

### Nếu gặp vấn đề với GUI:

Có thể chạy client từ Windows thông thường (Python trên Windows) kết nối đến server trong WSL:
- Server IP: `localhost` hoặc `127.0.0.1`
- Port: `5555`

## 7. Kiểm tra kết nối

```bash
# Kiểm tra server đã chạy chưa
ps aux | grep server

# Kiểm tra port
netstat -tulpn | grep 5555

# Hoặc
ss -tulpn | grep 5555
```

## 8. Dọn dẹp

```bash
make clean
```

## Troubleshooting

### Lỗi "Permission denied" khi chạy server:
```bash
chmod +x bin/server
./bin/server
```

### Lỗi "cannot open shared object file":
```bash
# Cài đặt thư viện thiếu
sudo apt install -y libpthread-stubs0-dev

# Hoặc kiểm tra ldd
ldd bin/server
ldd bin/client_network.so
```

### Lỗi "Display not found" khi chạy Python GUI:
```bash
# Kiểm tra DISPLAY
echo $DISPLAY

# Test X server
xclock  # Nếu hiển thị đồng hồ = OK
xeyes   # Nếu hiển thị đôi mắt = OK

# Cài đặt nếu thiếu
sudo apt install -y x11-apps
```

### Không kết nối được đến server từ client:
```bash
# Kiểm tra firewall trong WSL
sudo ufw status

# Tắt firewall nếu cần (chỉ để test)
sudo ufw disable

# Kiểm tra Windows Firewall
# Có thể cần thêm rule cho WSL
```

## Ghi chú quan trọng

1. **File paths**: WSL sử dụng `/` thay vì `\`
2. **Line endings**: Nếu copy file từ Windows, có thể cần convert:
   ```bash
   sudo apt install dos2unix
   find . -name "*.c" -o -name "*.h" -o -name "*.py" | xargs dos2unix
   ```
3. **Performance**: Truy cập file trong filesystem của WSL (`~/`) nhanh hơn file trên `/mnt/`
4. **Network**: Server trong WSL có thể truy cập từ Windows qua `localhost`

## Tính năng cross-platform

Dự án đã được chuyển đổi hoàn toàn sang Linux/POSIX:
- ✅ Thay Winsock → POSIX sockets
- ✅ Thay CreateThread → pthread
- ✅ Thay CRITICAL_SECTION → pthread_mutex_t
- ✅ Thay DLL (.dll) → Shared Object (.so)
- ✅ Makefile tự động phát hiện hệ điều hành

Dự án có thể chạy trên:
- ✅ Linux (Ubuntu, Debian, Fedora, Arch, etc.)
- ✅ WSL1 và WSL2
- ✅ macOS
- ⚠️  Windows (cần MinGW hoặc compile riêng với Winsock)
