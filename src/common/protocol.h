#ifndef PROTOCOL_H
#define PROTOCOL_H

// Auth & Lobby
#define MSG_TYPE_REGISTER       "REGISTER"
#define MSG_TYPE_REGISTER_SUCCESS "REGISTER_SUCCESS"
#define MSG_TYPE_REGISTER_FAIL  "REGISTER_FAIL"
#define MSG_TYPE_LOGIN          "LOGIN"
#define MSG_TYPE_LOGIN_SUCCESS  "LOGIN_SUCCESS"
#define MSG_TYPE_LOGIN_FAIL     "LOGIN_FAIL"
#define MSG_TYPE_LOGOUT         "LOGOUT"
#define MSG_TYPE_LOGOUT_SUCCESS "LOGOUT_SUCCESS"
#define MSG_TYPE_GET_LOBBY_LIST "GET_LOBBY_LIST"
#define MSG_TYPE_GET_LEADERBOARD "GET_LEADERBOARD"
#define MSG_TYPE_LEADERBOARD_DATA "LEADERBOARD_DATA"
#define MSG_TYPE_LOBBY_LIST       "LOBBY_LIST"

// Polling (Cơ chế để Client hỏi Server có biến gì không)
#define MSG_TYPE_POLL           "POLL"
#define MSG_TYPE_NO_EVENT       "NO_EVENT"

// Classic Mode Flow
#define MSG_TYPE_START_CLASSIC  "START_CLASSIC" // Yêu cầu bắt đầu chơi đơn

// Lifelines
#define MSG_TYPE_USE_LIFELINE   "USE_LIFELINE"  // Client yêu cầu dùng quyền trợ giúp
#define MSG_TYPE_LIFELINE_RES   "LIFELINE_RES"  // Server trả về kết quả trợ giúp

// Lifeline ID
#define LIFELINE_5050           1
#define LIFELINE_AUDIENCE       2
#define LIFELINE_CALL           3
#define LIFELINE_SWAP           4

// PvP Flow
#define MSG_TYPE_INVITE_PLAYER  "INVITE_PLAYER"  // A gửi yêu cầu mời B
#define MSG_TYPE_RECEIVE_INVITE "RECEIVE_INVITE" // B nhận được tin mời
#define MSG_TYPE_ACCEPT_INVITE  "ACCEPT_INVITE"  // B đồng ý
#define MSG_TYPE_REJECT_INVITE  "REJECT_INVITE"  // B từ chối
#define MSG_TYPE_INVITE_FAIL    "INVITE_FAIL"    // B đang bận
#define MSG_TYPE_GAME_START     "GAME_START"     // Server báo cả 2 bắt đầu

// Game Flow - Combat Phase
#define MSG_TYPE_REQUEST_QUESTION "REQUEST_QUESTION" // Client xin câu hỏi
#define MSG_TYPE_QUESTION       "QUESTION"       // Server gửi câu hỏi
#define MSG_TYPE_SUBMIT_ANSWER  "SUBMIT_ANSWER"  // Client gửi đáp án + thời gian
#define MSG_TYPE_ANSWER_RESULT  "ANSWER_RESULT"  // Server báo đúng/sai + điểm
#define MSG_TYPE_UPDATE_SCORE   "UPDATE_SCORE"   // Server cập nhật điểm 2 bên
#define MSG_TYPE_GAME_END       "GAME_END"       // Server báo kết thúc trận + kết quả
#define MSG_TYPE_CHECK_GAME_STATUS "CHECK_GAME_STATUS" // Client kiểm tra trạng thái game
#define MSG_TYPE_GAME_STATUS_UPDATE "GAME_STATUS_UPDATE" // Server trả về trạng thái game
#define MSG_TYPE_QUIT_GAME      "QUIT_GAME"      // Client thoát game

// History
#define MSG_TYPE_GET_HISTORY    "GET_HISTORY"    // Client lấy lịch sử đấu
#define MSG_TYPE_HISTORY_DATA   "HISTORY_DATA"   // Server trả về lịch sử

// Chat
#define MSG_TYPE_SEND_CHAT      "SEND_CHAT"         // Client gửi tin nhắn
#define MSG_TYPE_CHAT_SUCCESS   "CHAT_SUCCESS"      // Server xác nhận
#define MSG_TYPE_GET_CHAT_HISTORY "GET_CHAT_HISTORY" // Client lấy lịch sử chat
#define MSG_TYPE_CHAT_HISTORY   "CHAT_HISTORY"      // Server trả lịch sử
#define MSG_TYPE_NEW_CHAT_MESSAGE "NEW_CHAT_MESSAGE" // Server broadcast tin mới

// Buffer size
#define BUFFER_SIZE 4096

// Game Config
#define MAX_QUESTIONS_PER_GAME 15 // Tối đa câu hỏi trong 1 trận
#define MAX_TIME_PER_QUESTION 120  // Thời gian tối đa mỗi câu (giây)
#define BASE_SCORE 100            // Điểm cơ bản khi trả lời đúng

#endif // PROTOCOL_H