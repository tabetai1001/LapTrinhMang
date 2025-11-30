# Quiz Game - Trò chơi đố vui trực tuyến

Ứng dụng trò chơi đố vui trực tuyến với kiến trúc client-server, hỗ trợ nhiều người chơi cùng lúc.

## Yêu cầu hệ thống

- **MSYS2/MinGW-w64**: Để biên dịch mã nguồn C
- **Python 3.x**: Để chạy giao diện client
- **Windows**: Ứng dụng sử dụng Windows Sockets

## Cài đặt

### 1. Cài đặt MSYS2 MinGW-w64

Tải và cài đặt MSYS2 từ: https://www.msys2.org/

Sau khi cài đặt, mở MSYS2 MinGW 64-bit terminal và cài đặt các công cụ cần thiết:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

### 2. Cài đặt Python

Tải và cài đặt Python 3.x từ: https://www.python.org/downloads/

Đảm bảo Python đã được thêm vào PATH.

## Biên dịch chương trình

Mở **MSYS2 MinGW 64-bit** terminal, di chuyển đến thư mục dự án và chạy:

```bash
cd /d/20251/mili
make
```

Lệnh này sẽ biên dịch:
- `server.exe`: Chương trình server
- `client_network.dll`: Thư viện mạng cho client Python

## Chạy chương trình

### Bước 1: Khởi động Server

Trong terminal MSYS2 MinGW 64-bit:

```bash
./server.exe 5555
```

Server sẽ lắng nghe trên cổng 5555. Bạn sẽ thấy thông báo:
```
Server listening on port 5555...
```

### Bước 2: Khởi động Client

Mở **3 terminal riêng biệt** (có thể dùng Command Prompt, PowerShell, hoặc MSYS2) và chạy:

**Terminal 1:**
```bash
python client_gui.py
```

**Terminal 2:**
```bash
python client_gui.py
```

**Terminal 3:**
```bash
python client_gui.py
```

### Bước 3: Đăng nhập

Sử dụng các tài khoản sau để đăng nhập:

| Tên đăng nhập | Mật khẩu |
|---------------|----------|
| khue          | 123      |
| tung          | 123      |
| bach          | 123      |
| admin         | admin    |

## Hướng dẫn sử dụng

### Màn hình đăng nhập
- Nhập tên đăng nhập và mật khẩu
- Nhấn **Đăng nhập** để vào game
- Nhấn **Đăng ký** nếu muốn tạo tài khoản mới

### Màn hình chính (Lobby)
- **Danh sách người chơi**: 
  - 🟢 Xanh lá: Người chơi đang rảnh (FREE)
  - 🔴 Đỏ: Người chơi đang trong trận (IN_GAME)
  - ⚪ Xám: Người chơi offline (OFFLINE)
- **Mời đấu**: Nhấn vào tên người chơi rảnh để gửi lời mời
- **Lịch sử**: Xem các trận đấu đã chơi
- **Đăng xuất**: Thoát khỏi tài khoản

### Trong trận đấu
- Mỗi người chơi có **20 giây** để trả lời mỗi câu hỏi
- Chọn đáp án A, B, C, hoặc D
- Người trả lời đúng và nhanh hơn sẽ được điểm
- Trận đấu gồm nhiều câu hỏi cho đến khi hết câu

### Kết thúc trận đấu
- Xem điểm số và kết quả chi tiết
- Nhấn **OK** để quay về lobby

## Cấu trúc dự án

```
mili/
├── server.c              # Mã nguồn server
├── client_gui.py         # Giao diện client (Tkinter)
├── client_network.c      # Thư viện mạng client (DLL)
├── protocol.h            # Định nghĩa giao thức
├── cJSON.c               # Thư viện xử lý JSON
├── cJSON.h
├── Makefile              # File build
├── data/
│   ├── accounts.json     # Dữ liệu tài khoản
│   ├── questions.json    # Ngân hàng câu hỏi
│   └── history.json      # Lịch sử trận đấu
└── README.md
```

## Giao thức

Ứng dụng sử dụng giao thức JSON qua TCP Socket với các loại message:

- `LOGIN`: Đăng nhập
- `REGISTER`: Đăng ký tài khoản mới
- `LOGOUT`: Đăng xuất
- `POLL`: Client polling để nhận cập nhật
- `LOBBY_LIST`: Server gửi danh sách người chơi
- `INVITE`: Gửi lời mời đấu
- `ACCEPT_INVITE`: Chấp nhận lời mời
- `DECLINE_INVITE`: Từ chối lời mời
- `GAME_START`: Bắt đầu trận đấu
- `QUESTION`: Server gửi câu hỏi
- `ANSWER`: Client gửi câu trả lời
- `GAME_END`: Kết thúc trận đấu

## Tính năng

### Đã hoàn thành
- ✅ Đăng nhập/Đăng ký/Đăng xuất
- ✅ Lobby hiển thị tất cả người chơi với trạng thái real-time
- ✅ Mời đấu và chấp nhận/từ chối lời mời
- ✅ Hệ thống câu hỏi với giới hạn thời gian
- ✅ Tính điểm theo độ chính xác và tốc độ
- ✅ Lịch sử trận đấu
- ✅ Cập nhật lobby tự động khi có thay đổi
- ✅ Hỗ trợ cuộn chuột trong các danh sách
- ✅ Ẩn mật khẩu khi nhập

### Cơ chế cập nhật real-time
Server sử dụng **version-based polling**:
- Mỗi thay đổi trong lobby (login, logout, bắt đầu game, kết thúc game) tăng `lobby_version`
- Client gửi POLL mỗi giây kèm `lobby_version` hiện tại
- Server chỉ gửi LOBBY_LIST khi có thay đổi (version khác nhau)
- Tối ưu băng thông và CPU

## Xử lý lỗi thường gặp

### Server không khởi động được
- Kiểm tra port 5555 có đang được sử dụng không
- Chạy với quyền Administrator nếu cần

### Client không kết nối được
- Đảm bảo server đang chạy
- Kiểm tra file `client_network.dll` có trong thư mục không
- Kiểm tra firewall có chặn kết nối không

### Lỗi biên dịch
- Đảm bảo đang sử dụng **MSYS2 MinGW 64-bit** terminal
- Cài đặt đầy đủ gcc và make
- Chạy `make clean` rồi `make` lại

## Tác giả

Dự án môn Lập trình mạng - 2025

## License

Educational purposes only.
