# AI LÀ TRIỆU PHÚ - MULTIPLAYER GAME

## 📋 Mô tả dự án
Game "Ai Là Triệu Phú" đa người chơi với kiến trúc Client-Server, hỗ trợ chơi PvP (Player vs Player) và Classic mode.

---

## 🛠️ CÔNG NGHỆ SỬ DỤNG

### Kiến trúc hệ thống
- **Mô hình:** Client-Server Architecture
- **Giao thức:** TCP/IP (Transmission Control Protocol)
- **Mô hình lập trình:** Multi-threading (Đa luồng)

### Server Side (C Language)

#### 1. Socket Programming
**Windows Sockets API (Winsock2)**
- `socket()`: Tạo socket
- `bind()`: Gán địa chỉ IP và port
- `listen()`: Lắng nghe kết nối
- `accept()`: Chấp nhận kết nối từ client
- `recv()` / `send()`: Nhận/gửi dữ liệu
- `closesocket()`: Đóng kết nối

#### 2. Multi-threading & Concurrency
**Thread Management:**
- `CreateThread()`: Tạo thread mới cho mỗi client
- Thread pool để xử lý nhiều client đồng thời

**Synchronization (Đồng bộ hóa):**
- `CRITICAL_SECTION`: Bảo vệ tài nguyên dùng chung
- `EnterCriticalSection()` / `LeaveCriticalSection()`
- Áp dụng cho: game sessions, chat messages, client list, file I/O

#### 3. Data Structures
- **Circular Buffer:** Lưu trữ chat history (100 tin nhắn)
- **Array-based:** Quản lý danh sách client (MAX 100)
- **Linked structures:** Game sessions management

#### 4. File I/O
**JSON Processing:**
- Đọc/ghi file `accounts.json` (tài khoản người chơi)
- Đọc/ghi file `history.json` (lịch sử trận đấu)
- Đọc file `questions.json` (ngân hàng câu hỏi)
- **File Locking:** Đảm bảo tính toàn vẹn dữ liệu khi ghi file

#### 5. Protocol Design
**Custom Application Protocol:**
- Message-based communication
- Request-Response pattern
- JSON format cho data serialization
- 30+ message types khác nhau

### Client Side (Python)

#### 1. Socket Programming
**Python socket module:**
- `socket.socket(AF_INET, SOCK_STREAM)`
- `connect()`, `send()`, `recv()`
- Timeout handling
- Connection retry mechanism

#### 2. GUI Framework
**Tkinter:**
- Event-driven programming
- MVC pattern (Model-View-Controller)
- Responsive UI design (Grid layout)
- Custom widgets và styling

#### 3. Asynchronous Communication
**Polling Mechanism:**
- `after()` method cho periodic updates
- Non-blocking UI updates
- Real-time data synchronization
- Polling interval: 1000ms

#### 4. JSON Processing
**json module:**
- Encoding/Decoding messages
- Data serialization/deserialization

### Networking Concepts

#### 1. Connection Management
- **Persistent Connection:** Duy trì kết nối TCP
- **Session Management:** Session ID tracking
- **Connection Pooling:** Tái sử dụng kết nối

#### 2. Error Handling
**Network Errors:**
- Timeout handling
- Connection refused
- Socket errors
- Broken pipe

**Application Errors:**
- Invalid session
- Authentication failed
- Data validation errors
- Game state conflicts

#### 3. Security Considerations
**Session-based Authentication:**
- Session ID generation
- Session validation per request
- Automatic session cleanup

### Performance Optimization
- **Buffer Management:** Tối ưu send/recv buffer (4096 bytes)
- **Thread Pooling:** Giảm overhead tạo thread
- **In-memory Caching:** Cache game data và user info
- **Circular Buffer:** Efficient chat history management

---

## 📁 Cấu trúc thư mục

```
LapTrinhMang/
├── src/
│   ├── common/
│   │   ├── cJSON.c              # JSON parser library
│   │   ├── cJSON.h
│   │   └── protocol.h           # Protocol message definitions
│   │
│   ├── server/
│   │   ├── main.c               # Server entry point
│   │   ├── modules/
│   │   │   ├── connection_handler.c  # Xử lý kết nối & protocol
│   │   │   ├── game_service.c        # Logic trò chơi
│   │   │   ├── auth_service.c        # Xác thực & phân quyền
│   │   │   ├── data_manager.c        # Quản lý file JSON
│   │   │   └── server_state.c        # Quản lý trạng thái server
│   │   └── include/
│   │       ├── connection_handler.h
│   │       ├── game_service.h
│   │       ├── auth_service.h
│   │       ├── data_manager.h
│   │       ├── server_state.h
│   │       └── models.h              # Cấu trúc dữ liệu
│   │
│   └── client/
│       ├── main.py                   # Client entry point
│       ├── core/
│       │   ├── config.py             # UI colors & constants
│       │   └── network.py            # Network manager
│       ├── native/
│       │   └── client_network.c      # (Optional) Native DLL
│       └── ui/
│           ├── view_auth.py          # Đăng nhập/Đăng ký
│           ├── view_lobby.py         # Sảnh chờ & chat
│           ├── view_game.py          # Màn hình chơi game
│           ├── view_history.py       # Lịch sử trận đấu
│           └── widgets.py            # Custom UI components
│
├── data/
│   ├── accounts.json                 # Dữ liệu tài khoản
│   ├── questions.json                # Ngân hàng câu hỏi
│   └── history.json                  # Lịch sử trận đấu
│
├── bin/                              # Compiled executables
│   ├── server.exe                    # Server binary
│   └── client_network.dll            # (Optional) Client DLL
│
├── tests/                            # Unit tests
│   └── test_lobby_list.py
│
├── docs/                             # Documentation
│   ├── README.md
│   ├── README_THACH_DAU.md
│   └── SERVER_UPDATE_REQUIREMENTS.md
│
├── Makefile                          # Build script
├── config.ini                        # Cấu hình kết nối
└── crawler.py                        # Data crawler (optional)
```

---

## 🚀 Hướng dẫn cài đặt

### Yêu cầu hệ thống

#### Server
- **OS:** Windows 10/11
- **Compiler:** MinGW-w64 (gcc)
- **Build Tool:** Make
- **Libraries:** Winsock2 (ws2_32)

#### Client
- **Python:** 3.11 trở lên
- **Packages:** 
  - tkinter (thường có sẵn với Python)
  - json (built-in)
  - socket (built-in)

### Cài đặt Server

#### 1. Cài đặt MinGW-w64
```bash
# Download từ: https://www.mingw-w64.org/
# Hoặc dùng MSYS2:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

#### 2. Compile Server
```bash
# Di chuyển vào thư mục dự án
cd d:\20251\mili2\LapTrinhMang

# Clean build files
make clean

# Build server
make

# Kết quả: bin/server.exe
```

#### 3. Chạy Server
```bash
# Chạy server trên port 5555
./bin/server.exe 5555

# Output:
# === AI LA TRIEU PHU SERVER ===
# [State] Server state initialized.
# [Data] Loaded 100 questions into memory.
# [Server] Dang chay tai port 5555...
```

### Cài đặt Client

#### 1. Cài đặt Python
```bash
# Download Python từ: https://www.python.org/downloads/
# Đảm bảo check "Add Python to PATH" khi cài đặt
```

#### 2. Kiểm tra dependencies
```bash
# Kiểm tra Python version
python --version

# Kiểm tra tkinter
python -m tkinter
```

#### 3. Chạy Client
```bash
# Di chuyển vào thư mục client
cd src/client

# Chạy client
python main.py
```

---

## 🌐 Cấu hình kết nối nhiều máy

### Bước 1: Chuẩn bị máy Server

#### Lấy IP của máy Server
```cmd
ipconfig
```
Tìm dòng `IPv4 Address` (ví dụ: `192.168.1.100`)

#### Mở Firewall cho port 5555
```cmd
# Thêm rule cho Firewall
netsh advfirewall firewall add rule name="AI LA TRIEU PHU Server" dir=in action=allow protocol=TCP localport=5555

# Kiểm tra rule
netsh advfirewall firewall show rule name="AI LA TRIEU PHU Server"
```

### Bước 2: Cấu hình Client

#### Tạo file config.ini
Tạo file `config.ini` trong thư mục gốc dự án:

```ini
[SERVER]
host = 192.168.1.100
port = 5555
```

#### Sửa file network.py
File: `src/client/core/network.py`

```python
import configparser
import os

class NetworkManager:
    def __init__(self):
        # Đọc config từ file
        config = configparser.ConfigParser()
        config_path = os.path.join(os.path.dirname(__file__), '..', '..', 'config.ini')
        
        if os.path.exists(config_path):
            config.read(config_path)
            self.server_ip = config.get('SERVER', 'host', fallback='localhost')
            self.server_port = config.getint('SERVER', 'port', fallback=5555)
        else:
            # Fallback nếu không có file config
            self.server_ip = "localhost"
            self.server_port = 5555
        
        self.socket = None
        # ...existing code...
```

### Bước 3: Kiểm tra kết nối

#### Test connectivity từ máy Client
```bash
# Dùng ping
ping 192.168.1.100

# Dùng telnet (nếu có)
telnet 192.168.1.100 5555

# Dùng PowerShell
Test-NetConnection -ComputerName 192.168.1.100 -Port 5555
```

### Bước 4: Chạy thử nghiệm

1. **Trên máy Server:**
```bash
./bin/server.exe 5555
```

2. **Trên các máy Client:**
```bash
python src/client/main.py
```

### Kết nối qua Internet (WAN)

#### 1. Port Forwarding trên Router
- Đăng nhập vào Router (thường là `192.168.1.1`)
- Tìm mục "Port Forwarding" hoặc "Virtual Server"
- Thêm rule:
  - **External Port:** 5555
  - **Internal Port:** 5555
  - **Internal IP:** 192.168.1.100 (IP máy server)
  - **Protocol:** TCP

#### 2. Lấy IP Public
- Truy cập: https://whatismyip.com
- Client dùng IP Public này để kết nối

#### 3. Dynamic DNS (Khuyến nghị)
- Đăng ký dịch vụ: No-IP, DynDNS
- Tạo domain name trỏ đến IP Public
- Client dùng domain thay vì IP

---

## 📊 Luồng sự kiện trong trò chơi

### 1️⃣ ĐĂNG NHẬP / ĐĂNG KÝ

```
┌─────────┐                                    ┌─────────┐
│ Client  │                                    │ Server  │
└────┬────┘                                    └────┬────┘
     │                                              │
     │  REGISTER/LOGIN {username, password}        │
     │─────────────────────────────────────────────>│
     │                                              │
     │              Validate credentials            │
     │              Generate session_id             │
     │              Load user data from JSON        │
     │                                              │
     │  <─────────────────────────────────────────  │
     │   SUCCESS {session_id, cumulative_score}     │
     │                                              │
     │         Start polling every 1s               │
     │<─────────────────────────────────────────────│
     │                                              │
```

**Chi tiết:**
1. Client gửi username + password
2. Server kiểm tra trong `accounts.json`
3. Nếu hợp lệ: Tạo session_id, lưu vào ClientState
4. Trả về session_id và điểm tích lũy
5. Client bắt đầu polling để nhận sự kiện real-time

---

### 2️⃣ SẢNH CHỜ (LOBBY)

```
┌─────────┐                                    ┌─────────┐
│ Client  │                                    │ Server  │
└────┬────┘                                    └────┬────┘
     │                                              │
     │  GET_LOBBY_LIST                              │
     │─────────────────────────────────────────────>│
     │                                              │
     │  <─────────────────────────────────────────  │
     │   LOBBY_LIST {players: [{name, status},...]} │
     │                                              │
     │  SEND_CHAT {message}                         │
     │─────────────────────────────────────────────>│
     │                                              │
     │  <─────────────────────────────────────────  │
     │   CHAT_SUCCESS                                │
     │                                              │
     │  Polling every 1s                            │
     │  <─────────────────────────────────────────  │
     │   NEW_CHAT_MESSAGE {username, message}       │
     │                                              │
```

**Chi tiết:**
1. Client request danh sách người chơi (FREE/IN_GAME)
2. Server trả về list với trạng thái real-time
3. Client có thể chat, tin nhắn broadcast đến tất cả
4. Polling tự động cập nhật danh sách và chat mới

---

### 3️⃣ THÁCH ĐẤU (INVITE PLAYER)

```
┌──────────┐                                  ┌─────────┐                                  ┌──────────┐
│ Client A │                                  │ Server  │                                  │ Client B │
└────┬─────┘                                  └────┬────┘                                  └────┬─────┘
     │                                             │                                             │
     │  INVITE_PLAYER {target, num_questions}      │                                             │
     │────────────────────────────────────────────>│                                             │
     │                                             │  Set pending_invite_from                    │
     │  <──────────────────────────────────────────│                                             │
     │   INVITE_SENT_SUCCESS                       │                                             │
     │                                             │                                             │
     │                                             │  Polling                                    │
     │                                             │<────────────────────────────────────────────│
     │                                             │  RECEIVE_INVITE {from: "A", num_q: 5}       │
     │                                             │─────────────────────────────────────────────>│
     │                                             │                                             │
     │                                             │  ACCEPT_INVITE {from: "A"}                  │
     │                                             │<────────────────────────────────────────────│
     │                                             │  Create game session                        │
     │                                             │  Set both players BUSY                      │
     │                                             │  Load random questions                      │
     │                                             │                                             │
     │  Polling                                    │                                             │
     │<────────────────────────────────────────────│                                             │
     │   GAME_START {opponent, mode: "PVP"}        │                                             │
     │                                             │─────────────────────────────────────────────>│
     │                                             │   GAME_START {opponent, mode: "PVP"}        │
     │                                             │                                             │
```

**Chi tiết:**
1. Player A chọn Player B và gửi lời mời (5 câu mặc định)
2. Server set `pending_invite_from` cho B
3. Player B nhận thông báo qua polling
4. Player B accept → Server tạo game session
5. Cả 2 player nhận GAME_START và chuyển vào game

---

### 4️⃣ TRONG TRẬN ĐẤU (GAME SESSION)

```
┌──────────┐                                  ┌─────────┐                                  ┌──────────┐
│ Client A │                                  │ Server  │                                  │ Client B │
└────┬─────┘                                  └────┬────┘                                  └────┬─────┘
     │                                             │                                             │
     │  REQUEST_QUESTION                           │                                             │
     │────────────────────────────────────────────>│                                             │
     │                                             │  Get next question from bank                │
     │  <──────────────────────────────────────────│  Avoid duplicates                           │
     │   QUESTION {id, text, options, max_time}    │                                             │
     │                                             │                                             │
     │  [User answers within 15s]                  │                                             │
     │                                             │                                             │
     │  SUBMIT_ANSWER {q_id, answer_idx, time}     │                                             │
     │────────────────────────────────────────────>│                                             │
     │                                             │  Check correctness                          │
     │                                             │  Calculate score with time bonus            │
     │                                             │  Mark A answered                            │
     │  <──────────────────────────────────────────│                                             │
     │   ANSWER_RESULT {correct, earned_score}     │                                             │
     │                                             │                                             │
     │                                             │  SUBMIT_ANSWER {q_id, answer_idx, time}     │
     │                                             │<────────────────────────────────────────────│
     │                                             │  Check correctness                          │
     │                                             │  Calculate score                            │
     │                                             │  Mark B answered                            │
     │                                             │  Both answered → prepare next question      │
     │                                             │─────────────────────────────────────────────>│
     │                                             │   ANSWER_RESULT {correct, earned_score}     │
     │                                             │                                             │
     │  [Both polling]                             │                                             │
     │  <──────────────────────────────────────────│                                             │
     │   (via polling - next question ready)       │─────────────────────────────────────────────>│
     │                                             │                                             │
     │  ... Repeat for 5 questions ...             │                                             │
     │                                             │                                             │
     │  REQUEST_QUESTION (câu 6)                   │                                             │
     │────────────────────────────────────────────>│                                             │
     │  <──────────────────────────────────────────│                                             │
     │   NO_MORE_QUESTIONS                         │                                             │
     │                                             │                                             │
     │  CHECK_GAME_STATUS                          │                                             │
     │────────────────────────────────────────────>│                                             │
     │  <──────────────────────────────────────────│  Save to history.json                       │
     │   ANSWER_RESULT {game_status: FINISHED,     │  Update cumulative scores                   │
     │                  you_win: true,             │  Set both players FREE                      │
     │                  your_score, opp_score}     │─────────────────────────────────────────────>│
     │                                             │   ANSWER_RESULT {game_status: FINISHED,...} │
     │                                             │                                             │
```

**Chi tiết:**
1. Mỗi người chơi request câu hỏi độc lập
2. Server gửi câu hỏi (tránh trùng lặp)
3. Người chơi trả lời trong 15s
4. Server tính điểm:
   - Base score: 100 điểm
   - Time bonus: (15 - time_taken) * 10
   - Sai: 0 điểm
5. Khi cả 2 đã trả lời → sẵn sàng câu tiếp
6. Sau 5 câu → GAME_OVER
7. Lưu kết quả vào `history.json`

---

### 5️⃣ ĐẦU HÀNG (QUIT GAME)

```
┌──────────┐                                  ┌─────────┐                                  ┌──────────┐
│ Client A │                                  │ Server  │                                  │ Client B │
└────┬─────┘                                  └────┬────┘                                  └────┬─────┘
     │                                             │                                             │
     │  QUIT_GAME                                  │                                             │
     │────────────────────────────────────────────>│                                             │
     │                                             │  Is PvP? Yes                                │
     │                                             │  Save history (current scores)              │
     │                                             │  Set A: is_busy=0, status=FREE              │
     │                                             │  Set B: opponent_quit=1 (still BUSY)        │
     │  <──────────────────────────────────────────│                                             │
     │   QUIT_GAME_SUCCESS                         │                                             │
     │                                             │                                             │
     │  Back to Lobby (A is FREE)                  │                                             │
     │                                             │                                             │
     │                                             │  Polling                                    │
     │                                             │<────────────────────────────────────────────│
     │                                             │  OPPONENT_QUIT {opponent: "A"}              │
     │                                             │─────────────────────────────────────────────>│
     │                                             │                                             │
     │                                             │  Show popup "Đối thủ đã đầu hàng!"          │
     │                                             │  Display "Quay về sảnh chờ" button          │
     │                                             │  (B still IN_GAME until clicks button)      │
     │                                             │                                             │
     │                                             │  [User clicks button]                       │
     │                                             │  QUIT_GAME                                  │
     │                                             │<────────────────────────────────────────────│
     │                                             │  Set B: is_busy=0, status=FREE              │
     │                                             │─────────────────────────────────────────────>│
     │                                             │   QUIT_GAME_SUCCESS                         │
     │                                             │                                             │
     │                                             │  Back to Lobby (B is FREE)                  │
     │                                             │                                             │
```

**Chi tiết:**
1. Player A quit game (đầu hàng)
2. Server kiểm tra là trận PvP → lưu history với điểm hiện tại
3. A về FREE ngay lập tức
4. B nhận flag `opponent_quit=1`, vẫn BUSY
5. B nhận popup thông báo qua polling
6. B click "Quay về sảnh" → gửi QUIT_GAME
7. Server set B về FREE
8. Kết quả trong history: So sánh điểm số để xác định WIN/LOSE

---

### 6️⃣ CHƠI CLASSIC MODE

```
┌─────────┐                                    ┌─────────┐
│ Client  │                                    │ Server  │
└────┬────┘                                    └────┬────┘
     │                                              │
     │  START_CLASSIC                               │
     │─────────────────────────────────────────────>│
     │                                              │
     │  <─────────────────────────────────────────  │
     │   GAME_START {mode: "CLASSIC", opponent: ""}│
     │                                              │
     │  REQUEST_QUESTION                            │
     │─────────────────────────────────────────────>│
     │                                              │
     │  <─────────────────────────────────────────  │
     │   QUESTION {id, text, options}               │
     │                                              │
     │  SUBMIT_ANSWER {q_id, answer_idx, time}      │
     │─────────────────────────────────────────────>│
     │                                              │
     │  <─────────────────────────────────────────  │
     │   ANSWER_RESULT {correct, score}             │
     │                                              │
     │  [If correct] REQUEST_QUESTION               │
     │─────────────────────────────────────────────>│
     │                                              │
     │  ... Continue until wrong answer ...         │
     │                                              │
     │  SUBMIT_ANSWER (wrong answer)                │
     │─────────────────────────────────────────────>│
     │                                              │
     │  <─────────────────────────────────────────  │
     │   ANSWER_RESULT {correct: false,             │
     │                  game_status: FINISHED}      │
     │                                              │
     │  Display final score (no history saved)      │
     │                                              │
```

**Chi tiết:**
1. Player chọn Classic mode
2. Chơi cho đến khi trả lời sai
3. Không giới hạn số câu hỏi
4. Khi sai → GAME_OVER
5. **Không lưu vào history** (chỉ lưu điểm tích lũy)

---

### 7️⃣ XEM LỊCH SỬ

```
┌─────────┐                                    ┌─────────┐
│ Client  │                                    │ Server  │
└────┬────┘                                    └────┬────┘
     │                                              │
     │  GET_HISTORY                                 │
     │─────────────────────────────────────────────>│
     │                                              │
     │              Read history.json               │
     │              Filter user's games             │
     │              Calculate WIN/LOSE/DRAW         │
     │              Sort by timestamp               │
     │                                              │
     │  <─────────────────────────────────────────  │
     │   HISTORY_DATA {history: [{                  │
     │     game_key, player1, player2,              │
     │     score1, score2, total_questions,         │
     │     timestamp, result                        │
     │   },...]}                                    │
     │                                              │
     │  Display in ListView                         │
     │                                              │
```

**Chi tiết:**
1. Client request lịch sử cá nhân
2. Server đọc `history.json`
3. Lọc các trận có user tham gia
4. Tính WIN/LOSE:
   - Nếu `score1 > score2` → player1 WIN
   - Nếu `score2 > score1` → player2 WIN
   - Nếu bằng nhau → DRAW
   - Nếu `player2 = ""` → SOLO (Classic mode)
5. Trả về danh sách sorted theo timestamp

---

### 8️⃣ CHAT TRONG LOBBY

```
┌──────────┐                                  ┌─────────┐                                  ┌──────────┐
│ Client A │                                  │ Server  │                                  │ Client B │
└────┬─────┘                                  └────┬────┘                                  └────┬─────┘
     │                                             │                                             │
     │  SEND_CHAT {message: "Hello!"}              │                                             │
     │────────────────────────────────────────────>│                                             │
     │                                             │  Add to chat_messages[]                     │
     │                                             │  chat_version++                             │
     │  <──────────────────────────────────────────│                                             │
     │   CHAT_SUCCESS                              │                                             │
     │                                             │                                             │
     │                                             │  Polling                                    │
     │                                             │<────────────────────────────────────────────│
     │                                             │  Check last_chat_version                    │
     │                                             │  NEW_CHAT_MESSAGE                           │
     │                                             │─────────────────────────────────────────────>│
     │                                             │   {username: "A", message: "Hello!"}        │
     │                                             │                                             │
     │                                             │  Update chat UI                             │
     │                                             │                                             │
```

**Chi tiết:**
1. User gửi tin nhắn
2. Server lưu vào circular buffer (100 tin)
3. Tăng `chat_version`
4. Các client khác nhận qua polling
5. Auto-scroll xuống tin nhắn mới

---

### 9️⃣ POLLING MECHANISM

```
┌─────────┐                                    ┌─────────┐
│ Client  │                                    │ Server  │
└────┬────┘                                    └────┬────┘
     │                                              │
     │  Every 1000ms                                │
     │  POLL                                        │
     │─────────────────────────────────────────────>│
     │                                              │
     │  Server checks priorities:                   │
     │  1. opponent_quit flag                       │
     │  2. pending_invite_from                      │
     │  3. game_session_id (GAME_START)             │
     │  4. chat_version changed                     │
     │  5. lobby_version changed                    │
     │                                              │
     │  <─────────────────────────────────────────  │
     │   Event (highest priority)                   │
     │   OR NO_EVENT                                │
     │                                              │
     │  Client handles event                        │
     │  Schedule next poll after 1s                 │
     │                                              │
```

**Priorities:**
1. `opponent_quit=1` → OPPONENT_QUIT
2. `pending_invite_from != ""` → RECEIVE_INVITE
3. `is_busy=1 && game_session_id>=0` → GAME_START
4. `chat_version changed` → NEW_CHAT_MESSAGE
5. `lobby_version changed` → LOBBY_LIST
6. Else → NO_EVENT

---

## 📝 Protocol Messages

### Client → Server

| Message | Mô tả | Parameters |
|---------|-------|------------|
| `REGISTER` | Đăng ký tài khoản | `username`, `password` |
| `LOGIN` | Đăng nhập | `username`, `password` |
| `LOGOUT` | Đăng xuất | - |
| `GET_LOBBY_LIST` | Lấy danh sách người chơi | `include_offline` (optional) |
| `INVITE_PLAYER` | Gửi lời thách đấu | `target`, `num_questions` |
| `ACCEPT_INVITE` | Chấp nhận thách đấu | `from` |
| `REJECT_INVITE` | Từ chối thách đấu | `from` |
| `START_CLASSIC` | Bắt đầu chơi Classic | - |
| `REQUEST_QUESTION` | Yêu cầu câu hỏi | - |
| `SUBMIT_ANSWER` | Gửi câu trả lời | `question_id`, `answer_index`, `time_taken` |
| `USE_LIFELINE` | Sử dụng quyền trợ giúp | `lifeline_id` (1-4) |
| `CHECK_GAME_STATUS` | Kiểm tra trạng thái game | - |
| `QUIT_GAME` | Thoát game | - |
| `GET_HISTORY` | Xem lịch sử | - |
| `SEND_CHAT` | Gửi tin nhắn | `message` |
| `GET_CHAT_HISTORY` | Lấy lịch sử chat | - |
| `POLL` | Kiểm tra sự kiện mới | - |

### Server → Client

| Message | Mô tả | Data |
|---------|-------|------|
| `REGISTER_SUCCESS` | Đăng ký thành công | `message` |
| `REGISTER_FAIL` | Đăng ký thất bại | `message` |
| `LOGIN_SUCCESS` | Đăng nhập thành công | `user`, `total_score` |
| `LOGIN_FAIL` | Đăng nhập thất bại | `message` |
| `LOGOUT_SUCCESS` | Đăng xuất thành công | - |
| `LOBBY_LIST` | Danh sách người chơi | `players: [{name, status}]` |
| `INVITE_SENT_SUCCESS` | Đã gửi thách đấu | - |
| `INVITE_FAIL` | Gửi thách đấu thất bại | `message` |
| `RECEIVE_INVITE` | Nhận thách đấu | `from`, `num_questions` |
| `GAME_START` | Bắt đầu game | `opponent`, `mode`, `total_questions`, `game_key` |
| `QUESTION` | Câu hỏi | `question_id`, `question_number`, `question`, `options`, `max_time` |
| `NO_MORE_QUESTIONS` | Hết câu hỏi | - |
| `ANSWER_RESULT` | Kết quả câu trả lời | `is_correct`, `correct_answer`, `earned_score`, `your_total_score`, `opponent_score`, `game_status` |
| `LIFELINE_RES` | Kết quả trợ giúp | `data` (depends on lifeline) |
| `GAME_END` | Kết thúc game | `winner`, `score1`, `score2` |
| `OPPONENT_QUIT` | Đối thủ thoát | `opponent` |
| `QUIT_GAME_SUCCESS` | Thoát game thành công | - |
| `HISTORY_DATA` | Dữ liệu lịch sử | `history: [{game_key, player1, player2, score1, score2, total_questions, timestamp, result}]` |
| `CHAT_SUCCESS` | Gửi chat thành công | - |
| `CHAT_HISTORY` | Lịch sử chat | `messages: [{username, message, timestamp}]` |
| `NEW_CHAT_MESSAGE` | Tin nhắn mới | `username`, `message` |
| `NO_EVENT` | Không có sự kiện | - |
| `ERROR` | Lỗi | `message` |

---

## 🎮 Tính năng chính

### ✅ Xác thực & Quản lý tài khoản
- Đăng ký tài khoản mới
- Đăng nhập bằng username/password
- Session-based authentication
- Điểm tích lũy (cumulative score)

### ✅ Sảnh chờ (Lobby)
- Hiển thị danh sách người chơi online
- Trạng thái real-time (FREE/IN_GAME/OFFLINE)
- Auto-refresh khi có thay đổi
- Highlight player được chọn

### ✅ Hệ thống Chat
- Chat real-time giữa người chơi
- Lưu trữ 100 tin nhắn gần nhất
- Auto-scroll xuống tin mới
- Phân biệt tin của mình và người khác

### ✅ Thách đấu PvP
- Gửi lời mời thách đấu
- Accept/Reject invitation
- Chơi 5 câu hỏi ngẫu nhiên
- Tính điểm theo thời gian trả lời

### ✅ Chế độ Classic
- Chơi đơn không giới hạn
- Chơi đến khi trả lời sai
- Không lưu lịch sử (chỉ cộng điểm tích lũy)

### ✅ Hệ thống tính điểm
- **Base score:** 100 điểm/câu đúng
- **Time bonus:** (15 - time_taken) × 10
- **Trả lời sai:** 0 điểm
- **Công thức:** `score = correct ? (100 + (15 - time) * 10) : 0`

### ✅ Quyền trợ giúp (Lifelines)
- **50:50:** Loại bỏ 2 đáp án sai
- **Khán giả:** Hiện phần trăm bình chọn
- **Gọi điện:** Gợi ý đáp án từ "bạn bè"
- **Đổi câu:** Thay thế câu hỏi hiện tại

### ✅ Lịch sử trận đấu
- Lưu tất cả trận PvP
- Hiển thị: Đối thủ, Điểm số, Kết quả, Thời gian
- Tính WIN/LOSE dựa trên điểm số
- Sắp xếp theo timestamp

### ✅ Xử lý đầu hàng
- Người thoát về FREE ngay
- Người còn lại nhận thông báo
- Lưu lịch sử với điểm hiện tại
- Popup hiển thị ở giữa màn hình

### ✅ Responsive UI
- Grid layout tự động giãn
- 2 cột: Danh sách player | Chat
- Popup center-aligned
- Minimum window size: 900×650

---

## 🔒 Bảo mật & Xử lý lỗi

### Session Management
- Session ID được tạo khi login thành công
- Mọi request phải có session_id hợp lệ (implicit trong ClientState)
- Session tự động cleanup khi disconnect

### Error Handling

#### Network Errors
- **Timeout:** Socket timeout 5s
- **Connection refused:** Server offline
- **Broken pipe:** Connection lost
- **Retry mechanism:** Auto-reconnect khi mất kết nối

#### Application Errors
- **Invalid session:** Session không tồn tại hoặc đã logout
- **Game not found:** Game session không tồn tại
- **Player busy:** Người chơi đang trong trận
- **Invalid data:** JSON format sai

### Thread Safety

Critical Sections bảo vệ:
- **cs_clients:** Client list operations
- **cs_games:** Game session management
- **cs_history:** History read/write
- **cs_lobby:** Lobby version updates
- **cs_data:** File I/O operations
- **cs_chat:** Chat messages buffer

### Data Validation
- Kiểm tra JSON format
- Validate required fields
- Sanitize user input
- Prevent SQL injection (N/A - dùng JSON)

---

## 🐛 Known Issues & Limitations

### Server
- ❌ Tối đa 100 clients đồng thời (MAX_CLIENTS)
- ❌ Tối đa 100 trận đấu trong history
- ❌ Chat history giới hạn 100 tin nhắn
- ❌ Không có password encryption
- ❌ Không có rate limiting
- ❌ Single-threaded file I/O

### Client
- ❌ Không có auto-reconnect khi server restart
- ❌ Polling có thể miss events nếu quá nhiều
- ❌ Không cache questions locally
- ❌ GUI freeze nếu network slow

### Security
- ⚠️ Password lưu plain text trong JSON
- ⚠️ Không có SSL/TLS encryption
- ⚠️ Session ID predictable
- ⚠️ Không có input sanitization

---

## 🔄 Future Improvements

### High Priority
- [ ] Password hashing (bcrypt, SHA-256)
- [ ] SSL/TLS encryption cho traffic
- [ ] Auto-reconnect mechanism
- [ ] Database thay vì JSON files (SQLite)
- [ ] Rate limiting cho POLL requests

### Medium Priority
- [ ] Leaderboard (top players by score)
- [ ] Spectator mode (xem người khác chơi)
- [ ] Private rooms (tạo phòng riêng)
- [ ] More lifelines (phát lại video, tìm kiếm)
- [ ] Sound effects & music

### Low Priority
- [ ] Mobile app (Android/iOS)
- [ ] Web-based client (HTML5)
- [ ] Tournament system
- [ ] Achievements & badges
- [ ] Friend system

---

## 📚 Tài liệu tham khảo

### Socket Programming
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Winsock2 Documentation](https://docs.microsoft.com/en-us/windows/win32/winsock/)
- [Python Socket Programming HOWTO](https://docs.python.org/3/howto/sockets.html)

### Multi-threading
- [Windows Threading API](https://docs.microsoft.com/en-us/windows/win32/procthread/)
- [Critical Sections](https://docs.microsoft.com/en-us/windows/win32/sync/critical-section-objects)

### JSON Processing
- [cJSON Library](https://github.com/DaveGamble/cJSON)
- [Python json module](https://docs.python.org/3/library/json.html)

### GUI Development
- [Tkinter Documentation](https://docs.python.org/3/library/tkinter.html)
- [Tkinter Tutorial](https://realpython.com/python-gui-tkinter/)

---

## 👥 Đội ngũ phát triển

- **Sinh viên 1:** [Tên] - Server Development
- **Sinh viên 2:** [Tên] - Client Development
- **Sinh viên 3:** [Tên] - UI/UX Design
- **Giảng viên hướng dẫn:** [Tên]

---

## 📄 License

Dự án này được phát triển cho mục đích học tập trong môn Lập Trình Mạng.

**Trường:** [Tên trường]  
**Khoa:** [Tên khoa]  
**Năm học:** 2024-2025

---

## 📞 Liên hệ & Đóng góp

- **Email:** [email@example.com]
- **GitHub:** [https://github.com/tabetai1001/LapTrinhMang](https://github.com/tabetai1001/LapTrinhMang)
- **Issues:** Báo lỗi tại GitHub Issues

### Đóng góp (Contributing)
1. Fork repository
2. Tạo branch mới (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Tạo Pull Request

---

**Phiên bản:** 1.0.0  
**Ngày cập nhật:** December 1, 2025  
**Branch:** khue
