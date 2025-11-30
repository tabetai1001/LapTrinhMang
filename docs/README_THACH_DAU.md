# Hệ Thống Thách Đấu - Ai Là Triệu Phú

## Các Tính Năng Mới

### 1. Chế độ PvP (Thách Đấu 1v1)
- Hai người chơi đối đầu trực tiếp
- Mỗi trận đấu có **5 câu hỏi ngẫu nhiên**
- Câu hỏi được chọn từ bộ 15 câu với độ khó khác nhau (easy, medium, hard)

### 2. Hệ Thống Tính Điểm Thông Minh
**Công thức tính điểm:**
```
Điểm = BASE_SCORE × (1 - time_taken / MAX_TIME)
```

- **BASE_SCORE**: 100 điểm cơ bản
- **MAX_TIME**: 15 giây/câu
- Trả lời đúng + nhanh = điểm cao
- Trả lời đúng + chậm = điểm thấp
- Trả lời sai = 0 điểm

**Ví dụ:**
- Trả lời đúng sau 3 giây: 100 × (1 - 3/15) = 80 điểm
- Trả lời đúng sau 10 giây: 100 × (1 - 10/15) = 33 điểm
- Trả lời sai hoặc hết giờ: 0 điểm

### 3. Giao Diện Game Mới
- **Timer**: Đếm ngược thời gian còn lại
- **Bảng điểm**: Hiển thị điểm 2 bên real-time
- **Thông báo kết quả**: Đúng/Sai + điểm đạt được
- **Màn hình kết thúc**: Thông báo thắng/thua với tổng điểm

## Cách Chơi

### Bước 1: Mời thách đấu
1. Đăng nhập vào hệ thống
2. Xem danh sách người chơi online
3. Chọn người chơi muốn thách đấu
4. Nhấn nút **"THÁCH ĐẤU (PvP)"**

### Bước 2: Chấp nhận thách đấu
- Người được mời sẽ nhận popup
- Chọn **Yes** để chấp nhận hoặc **No** để từ chối

### Bước 3: Bắt đầu trận đấu
- Cả 2 người chơi cùng nhận câu hỏi
- Timer bắt đầu đếm ngược từ 15 giây
- Chọn đáp án A/B/C/D
- Xem kết quả ngay lập tức

### Bước 4: Tiếp tục đến hết 5 câu
- Sau mỗi câu, điểm được cập nhật
- Chuyển sang câu tiếp theo tự động
- Sau câu 5, hệ thống công bố kết quả

### Bước 5: Kết thúc
- Người có điểm cao hơn thắng
- Quay lại lobby để chơi tiếp

## Biên Dịch và Chạy

### Server (Windows)
```cmd
gcc -o server server.c cJSON.c -lws2_32
server.exe
```

### Client (Python)
```cmd
python client_gui.py
```

## Cấu Trúc Câu Hỏi Mới (questions.json)

Đã thêm 15 câu hỏi với các độ khó:
- **Easy**: Câu hỏi cơ bản (5 câu)
- **Medium**: Câu hỏi trung bình (5 câu)
- **Hard**: Câu hỏi khó (5 câu)

Ví dụ:
```json
{
  "id": 8,
  "question": "He dieu hanh Linux dau tien ra doi nam nao?",
  "options": ["1989", "1991", "1995", "2001"],
  "answer_index": 1,
  "difficulty": "hard"
}
```

## Protocol Mới

### MSG_TYPE_REQUEST_QUESTION
Client yêu cầu câu hỏi tiếp theo

### MSG_TYPE_QUESTION
Server gửi câu hỏi + options + thời gian

### MSG_TYPE_SUBMIT_ANSWER
Client gửi đáp án + thời gian đã dùng

### MSG_TYPE_ANSWER_RESULT
Server trả về:
- Đúng/Sai
- Điểm đạt được
- Tổng điểm hiện tại của 2 bên
- Trạng thái game (tiếp tục/kết thúc)

### MSG_TYPE_GAME_END
Thông báo kết thúc trận với kết quả thắng/thua

## Lưu Ý

1. **Thời gian**: Nếu không trả lời trong 15 giây, hệ thống tự động tính là sai
2. **Công bằng**: Cả 2 người chơi nhận cùng 1 câu hỏi cùng lúc
3. **Điểm số**: Được tính ngay sau mỗi câu và cập nhật cho cả 2 bên
4. **Kết thúc**: Sau 5 câu, người có điểm cao hơn thắng (nếu bằng điểm = hòa)

## Các File Đã Sửa Đổi

1. **protocol.h**: Thêm protocol mới cho game flow
2. **server.c**: Thêm GameSession struct + game logic + scoring system
3. **client_gui.py**: Thêm UI game với timer + score display
4. **data/questions.json**: Thêm 12 câu hỏi mới (tổng 15 câu)

## Công Nghệ Sử Dụng

- **Server**: C + WinSock2 + cJSON + Multi-threading
- **Client**: Python + Tkinter + ctypes
- **Protocol**: JSON-based messaging
- **Scoring**: Time-based algorithm

---

Chúc bạn chơi vui vẻ! 🎮🏆
