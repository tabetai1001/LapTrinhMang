# 🏗️ THIẾT KẾ HỆ THỐNG - AI LÀ TRIỆU PHÚ

## 1. Kiến trúc tổng thể

Hệ thống sử dụng mô hình **Client-Server 3-tier Architecture**:

```
┌──────────────────────────────────────────────────────────────┐
│                    PRESENTATION LAYER                         │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐ │
│  │  Tkinter GUI   │  │  Tkinter GUI   │  │  Tkinter GUI   │ │
│  │   (Client 1)   │  │   (Client 2)   │  │   (Client N)   │ │
│  └────────┬───────┘  └────────┬───────┘  └────────┬───────┘ │
└───────────┼──────────────────────────────────────────────────┘
            │                   │                   │
            └───────────────────┴───────────────────┘
                    TCP Socket (Port 5555)
            ┌───────────────────┬───────────────────┐
            │                   │                   │
┌───────────▼───────────────────▼───────────────────▼────────────┐
│                      APPLICATION LAYER                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              Main Thread (Accept Loop)                  │   │
│  └───────────────────────┬─────────────────────────────────┘   │
│                          │                                     │
│  ┌───────────────────────▼──────────────────────────────────┐  │
│  │       Connection Handler Threads (1 per client)          │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐                │  │
│  │  │ Thread 1 │  │ Thread 2 │  │ Thread N │                │  │
│  │  └─────┬────┘  └─────┬────┘  └─────┬────┘                │  │
│  └────────┼─────────────┼─────────────┼─────────────────────┘  │
│           │             │             │                        │
│  ┌────────▼─────────────▼─────────────▼──────────────────────┐ │
│  │           Business Logic Services                         │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │ │
│  │  │Auth Service  │  │Game Service  │  │Chat Service  │     │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘     │ │
│  └──────────────────────────┬────────────────────────────────┘ │
│                             │                                  │
│  ┌──────────────────────────▼────────────────────────────────┐ │
│  │          Server State (Shared Memory)                     │ │
│  │  - Client List                                            │ │
│  │  - Game Sessions                                          │ │
│  │  - Chat History                                           │ │
│  │  - Question Bank [500 questions]                          │ │
│  │  Protected by: pthread_mutex_t                            │ │
│  └───────────────────────────┬───────────────────────────────┘ │
└────────────────────────────────────────────────────────────────┘
                               │
┌──────────────────────────────▼────────────────────────────────┐
│                        DATA LAYER                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │accounts.json │  │questions.json│  │ history.json │         │
│  │  User data   │  │ 500+ Q&A     │  │ Match history│         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
└───────────────────────────────────────────────────────────────┘
```

## 2. Các thành phần chính

### 2.1. Client Side (Python + Tkinter)

**Cấu trúc module:**
```
src/client/
├── main.py              # Entry point, UI controller
├── core/
│   ├── network.py       # NetworkManager - socket communication
│   └── config.py        # UI colors, constants
├── ui/
│   ├── view_auth.py     # Login/Register screen
│   ├── view_lobby.py    # Lobby, player list, chat
│   ├── view_game.py     # Game screen, questions, lifelines
│   └── view_history.py  # Match history display
└── native/
    └── client_network_windows.c  # Windows socket wrapper
```

**Chức năng:**
- Kết nối TCP đến server (127.0.0.1:5555)
- Gửi request dạng JSON qua socket
- Polling server mỗi 1 giây để nhận updates
- Hiển thị UI responsive với Tkinter

**Các View chính:**
- **AuthView:** Login/Register form
- **LobbyView:** Danh sách người chơi, chat, leaderboard
- **GameView:** Hiển thị câu hỏi, đáp án, lifelines, timer
- **HistoryView:** Lịch sử thi đấu cá nhân

### 2.2. Server Side (C + pthread)

**Cấu trúc module:**
```
src/server/
├── main.c                    # Entry point, socket setup
├── include/
│   ├── models.h             # Data structures (ClientState, GameSession)
│   ├── server_state.h       # Global state & mutex
│   ├── connection_handler.h # Thread function, message routing
│   ├── auth_service.h       # Login/Register logic
│   ├── game_service.h       # Game logic, scoring
│   └── data_manager.h       # File I/O operations
└── modules/
    ├── connection_handler.c # Main request handler
    ├── auth_service.c       # Authentication
    ├── game_service.c       # Game mechanics
    ├── server_state.c       # State initialization
    └── data_manager.c       # JSON read/write
```

**Chức năng:**
- Lắng nghe port 5555
- Accept connections và spawn threads
- Xử lý 30+ loại message types
- Quản lý game sessions và scoring
- Đồng bộ hóa dữ liệu với mutex

**Các Service chính:**
- **Auth Service:** Xử lý register, login, logout
- **Game Service:** Tạo game session, lấy câu hỏi, tính điểm, lifelines
- **Chat Service:** Lưu và broadcast chat messages
- **Data Manager:** Read/write JSON files

### 2.3. Protocol Layer

**Message format (JSON):**
```json
{
  "type": "MESSAGE_TYPE",
  "data": {
    "field1": "value1",
    "field2": "value2"
  }
}
```

**Các nhóm message types:**

1. **Authentication:**
   - REGISTER, REGISTER_SUCCESS, REGISTER_FAIL
   - LOGIN, LOGIN_SUCCESS, LOGIN_FAIL
   - LOGOUT, LOGOUT_SUCCESS

2. **Lobby:**
   - GET_LOBBY_LIST, LOBBY_LIST
   - GET_LEADERBOARD, LEADERBOARD_DATA

3. **PvP:**
   - INVITE_PLAYER, RECEIVE_INVITE
   - ACCEPT_INVITE, REJECT_INVITE, INVITE_FAIL
   - GAME_START

4. **Game:**
   - START_CLASSIC
   - REQUEST_QUESTION, QUESTION
   - SUBMIT_ANSWER, ANSWER_RESULT
   - UPDATE_SCORE, GAME_END
   - USE_LIFELINE, LIFELINE_RES
   - QUIT_GAME

5. **Chat:**
   - SEND_CHAT, CHAT_SUCCESS
   - GET_CHAT_HISTORY, CHAT_HISTORY
   - NEW_CHAT_MESSAGE

6. **History:**
   - GET_HISTORY, HISTORY_DATA

7. **Polling:**
   - POLL, NO_EVENT

## 3. Data Structures

### 3.1. ClientState (Server-side)
```c
typedef struct {
    int socket;                    // Socket descriptor
    char username[50];             // Tên đăng nhập
    int is_logged_in;              // 0 hoặc 1
    int score;                     // Điểm tích lũy
    int is_busy;                   // Đang chơi game?
    char pending_invite_from[50];  // Ai đang mời?
    char current_opponent[50];     // Đang đấu với ai?
    int game_session_id;           // ID game hiện tại
    int current_question_index;    // Câu hỏi thứ mấy?
    int opponent_quit;             // Đối thủ bỏ cuộc?
    int last_lobby_version;        // Version lobby cuối
    int last_chat_version;         // Version chat cuối
} ClientState;
```

### 3.2. GameSession
```c
typedef struct {
    int id;                                // Session ID
    long long game_key;                    // Unique key
    char player1[50];                      // Người chơi 1
    char player2[50];                      // Người chơi 2 (rỗng = Classic)
    int score1, score2;                    // Điểm số
    int total_questions;                   // Số câu hỏi (15)
    int is_active;                         // Game đang chơi?
    int used_question_ids[15];             // Câu đã dùng
    int lifelines_used[2][4];              // Quyền trợ giúp đã dùng
} GameSession;
```

### 3.3. Question
```c
typedef struct {
    int id;                          // Question ID
    char question[512];              // Nội dung câu hỏi
    char options[4][256];            // 4 đáp án A, B, C, D
    int answer_index;                // 0-3 (đáp án đúng)
    int difficulty;                  // 1: Easy, 2: Medium, 3: Hard
    char category[100];              // Thể loại
} Question;
```

### 3.4. ChatMessage (Circular Buffer)
```c
typedef struct {
    char username[50];
    char message[256];
    time_t timestamp;
} ChatMessage;

ChatMessage chat_history[100];  // Circular buffer
int chat_count = 0;
```

## 4. Workflow chính

### 4.1. Connection Flow
```
Client                          Server
  │                               │
  ├─ socket() ──────────────────► │
  ├─ connect(5555) ─────────────► ├─ accept()
  │                               ├─ pthread_create()
  │◄────── CONNECTION_OK ─────────┤
  │                               │
  ├─ REGISTER/LOGIN ────────────► ├─ Verify credentials
  │◄────── LOGIN_SUCCESS ─────────┤ (with score)
  │                               │
  ├─ POLL (every 1s) ───────────► │
  │◄────── Events/NO_EVENT ───────┤
```

### 4.2. Classic Mode Flow
```
Client                          Server
  │                               │
  ├─ START_CLASSIC ─────────────► ├─ Create session (player2 = "")
  │◄────── GAME_START ────────────┤
  │                               │
  ├─ REQUEST_QUESTION ──────────► ├─ Get random question
  │◄────── QUESTION ──────────────┤
  │                               │
  ├─ SUBMIT_ANSWER + time ──────► ├─ Calculate score
  │◄────── ANSWER_RESULT ─────────┤
  │◄────── UPDATE_SCORE ──────────┤
  │                               │
  │    (Repeat 15 times)          │
  │                               │
  │◄────── GAME_END ───────────────┤ (Total score + save history)
```

### 4.3. PvP Mode Flow
```
Player A              Server              Player B
  │                     │                    │
  ├─ INVITE_PLAYER ───►│                    │
  │                     ├─ Check B status   │
  │                     ├─ RECEIVE_INVITE ─►│
  │                     │◄─ ACCEPT_INVITE ──┤
  │◄─ GAME_START ──────┤──── GAME_START ───►│
  │                     │                    │
  ├─ REQUEST_QUESTION ─►│                    │
  │◄─ QUESTION ─────────┤──── QUESTION ─────►│
  │                     │                    │
  ├─ SUBMIT_ANSWER ────►│◄─ SUBMIT_ANSWER ──┤
  │                     ├─ Calculate both    │
  │◄─ ANSWER_RESULT ────┤──── ANSWER_RESULT─►│
  │◄─ UPDATE_SCORE ─────┤──── UPDATE_SCORE ─►│
  │                     │                    │
  │    (Repeat 15 times)                     │
  │                     │                    │
  │◄─ GAME_END ─────────┤──── GAME_END ─────►│
  │    (Winner/Loser)   │    (Winner/Loser)  │
```

### 4.4. Lifeline Flow
```
Client                          Server
  │                               │
  │ (During game)                 │
  ├─ USE_LIFELINE (id=1) ───────► ├─ Check if used
  │                               ├─ Process 50:50
  │◄────── LIFELINE_RES ──────────┤ (2 wrong removed)
  │                               │
```

## 5. Concurrency Control

### 5.1. Synchronization Mechanisms

**Mutex Protection:**
```c
pthread_mutex_t state_mutex;
```

**Critical Sections:**
- Client list operations (add/remove/search)
- Game session create/update/delete
- Chat history circular buffer write
- File I/O (accounts.json, history.json)
- Lobby version increment

### 5.2. Thread Safety Pattern

```c
// Example: Add client to list
pthread_mutex_lock(&state_mutex);

for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].socket == 0) {
        clients[i] = new_client;
        break;
    }
}

pthread_mutex_unlock(&state_mutex);
```

### 5.3. Deadlock Prevention

- **Lock Ordering:** Luôn acquire mutex theo thứ tự cố định
- **No Nested Locks:** Tránh giữ 2 locks cùng lúc
- **Short Critical Sections:** Giữ lock trong thời gian ngắn nhất

## 6. Scoring Algorithm

```c
int calculate_score(int is_correct, double time_taken) {
    if (!is_correct) return 0;
    
    int base_score = 100;
    double time_factor = 1.0 - (time_taken / 120.0) * 0.5;
    // Trả lời càng nhanh, điểm càng cao
    // time_taken = 0s   → factor = 1.0  → 100 điểm
    // time_taken = 60s  → factor = 0.75 → 75 điểm
    // time_taken = 120s → factor = 0.5  → 50 điểm
    
    return (int)(base_score * time_factor);
}
```

## 7. Scalability & Limitations

### 7.1. Current Limits

| Resource            | Limit | Lý do                          |
|---------------------|-------|--------------------------------|
| Max clients         | 30    | Array-based client list        |
| Max game sessions   | 10    | Array-based session list       |
| Chat history        | 100   | Circular buffer size           |
| Question bank       | 500   | RAM limit                      |
| Questions per game  | 15    | Game design                    |

### 7.2. Performance Considerations

**Bottlenecks:**
- File I/O: Đọc/ghi JSON files (blocking)
- Mutex contention: Nhiều threads chờ lock
- Polling: Client poll mỗi 1 giây (bandwidth)

**Optimizations:**
- Question bank loaded vào RAM khi startup
- Chat history dùng circular buffer (O(1) write)
- Lobby updates chỉ gửi khi có thay đổi (version tracking)

### 7.3. Future Improvements

**Short-term:**
- Connection pooling
- Async file I/O
- Message queue cho broadcast

**Long-term:**
- Database integration (PostgreSQL)
- Redis cho caching
- Load balancing (multiple server instances)
- WebSocket cho real-time communication
- Microservices architecture

## 8. Security Considerations

**Implemented:**
- Password hashing (nên thêm bcrypt)
- Input validation (username/password length)
- Session management (socket-based)

**To Implement:**
- TLS/SSL encryption
- Rate limiting
- SQL injection prevention (khi dùng DB)
- CSRF protection
- Authentication tokens (JWT)

## 9. Testing Strategy

**Unit Testing:**
- Scoring algorithm
- Lifeline logic
- JSON parsing

**Integration Testing:**
- Client-Server communication
- Multi-threading concurrency
- File I/O operations

**Load Testing:**
- 30 concurrent clients
- 10 simultaneous games
- Chat message flood

**Manual Testing:**
- UI/UX testing
- Game flow testing
- Error handling

---

## 📊 Tổng kết

Hệ thống được thiết kế với các nguyên tắc:
- **Modularity:** Tách biệt concerns (UI, Network, Business Logic)
- **Scalability:** Có thể mở rộng với DB và load balancing
- **Concurrency:** Thread-safe với mutex protection
- **Maintainability:** Code structure rõ ràng, dễ debug
- **Performance:** Optimize với RAM cache và circular buffers
