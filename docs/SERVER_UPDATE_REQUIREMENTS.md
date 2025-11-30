# 🔧 YÊU CẦU CẬP NHẬT SERVER

## Để giao diện client hoạt động đúng, server cần hỗ trợ các tính năng sau:

### 1. **Hiển thị tất cả người chơi (bao gồm offline)**

**Request từ Client:**
```json
{
    "type": "GET_LOBBY_LIST",
    "include_offline": true
}
```

**Response từ Server:**
```json
{
    "type": "LOBBY_LIST",
    "players": [
        {
            "name": "player1",
            "status": "FREE"
        },
        {
            "name": "player2", 
            "status": "IN_GAME"
        },
        {
            "name": "player3",
            "status": "OFFLINE"
        }
    ]
}
```

**Status values:**
- `"FREE"` - Người chơi đang rảnh, có thể thách đấu
- `"IN_GAME"` - Người chơi đang trong trận
- `"OFFLINE"` - Người chơi đã offline

---

### 2. **Thông báo khi đối thủ bỏ cuộc**

**Request từ Client (khi quit game):**
```json
{
    "type": "QUIT_GAME",
    "game_key": 12345,
    "opponent": "opponent_name"
}
```

**Response gửi đến đối thủ (qua POLL):**
```json
{
    "type": "OPPONENT_QUIT",
    "opponent": "player_who_quit"
}
```

---

### 3. **Cập nhật trạng thái realtime**

**Server cần:**
1. Khi người chơi login → cập nhật status thành `"FREE"`
2. Khi người chơi vào trận → cập nhật status thành `"IN_GAME"`
3. Khi người chơi logout/disconnect → cập nhật status thành `"OFFLINE"`
4. Khi có thay đổi trong lobby → gửi `LOBBY_UPDATE` qua POLL

**Response LOBBY_UPDATE (optional, để cập nhật nhanh hơn):**
```json
{
    "type": "LOBBY_UPDATE",
    "message": "Player list has changed"
}
```

---

### 4. **Lưu trữ lịch sử người chơi**

Server cần lưu thông tin tất cả người chơi đã đăng ký, không chỉ người đang online, để có thể trả về danh sách đầy đủ khi client request với `include_offline: true`.

---

## 📝 Tóm tắt thay đổi cần thiết:

### ✅ **Bắt buộc:**
1. Trả về danh sách người chơi với status (FREE/IN_GAME/OFFLINE)
2. Xử lý QUIT_GAME và thông báo cho đối thủ
3. Cập nhật status người chơi khi vào/ra trận

### 🔄 **Tùy chọn (để tăng trải nghiệm):**
1. Gửi LOBBY_UPDATE khi có thay đổi (người mới login, đổi status, etc.)
2. Broadcast status changes để client không phải poll thường xuyên

---

## 🎯 Lợi ích:

1. **Người chơi thấy tất cả người chơi** - dễ dàng theo dõi ai đang online/offline
2. **Thông báo khi đối thủ bỏ cuộc** - trải nghiệm tốt hơn, không phải chờ mãi
3. **Cập nhật trạng thái realtime** - người chơi mới join sẽ xuất hiện ngay lập tức
4. **Màu sắc phân biệt** - dễ dàng nhận biết ai có thể thách đấu

---

## 🔍 Test Cases:

### Test 1: Hiển thị đầy đủ người chơi
- Có 3 người đăng ký: A, B, C
- Chỉ A và B online
- Client của A và B phải thấy cả 3 người, với C màu xám (offline)

### Test 2: Cập nhật trạng thái
- A và B đang ở lobby
- C login vào
- A và B phải thấy C xuất hiện trong danh sách ngay lập tức (trong vòng 2 giây)

### Test 3: Trạng thái trong trận
- A và B bắt đầu trận đấu
- C ở lobby phải thấy A và B chuyển sang màu đỏ (đang chơi)

### Test 4: Đối thủ bỏ cuộc
- A và B đang trong trận
- B ấn "Dừng cuộc chơi"
- A phải nhận được thông báo "Đối thủ B đã bỏ cuộc! Bạn thắng!"
- Hiển thị màn hình chiến thắng cho A

---

**Lưu ý:** Client sẽ tự động poll mỗi 1 giây và refresh lobby mỗi 2 giây khi không trong trận, do đó server cần đảm bảo hiệu suất tốt cho các request GET_LOBBY_LIST.
