#ifndef MODELS_H
#define MODELS_H

// QUAN TRỌNG: Winsock2 phải luôn được include TRƯỚC windows.h
#include <winsock2.h>
#include <windows.h> 
#include <time.h>

#define MAX_CLIENTS 30
#define MAX_GAME_SESSIONS 10
#define MAX_HISTORY 100
#define MAX_QUESTIONS_PER_GAME 15
#define BUFFER_SIZE 4096
#define MAX_QUESTIONS 500        // Số lượng câu hỏi tối đa nạp vào RAM
#define QUESTION_TEXT_SIZE 512   // Độ dài tối đa của câu hỏi
#define OPTION_TEXT_SIZE 256     // Độ dài tối đa của đáp án

typedef struct {
    int id;
    char question[QUESTION_TEXT_SIZE];
    char options[4][OPTION_TEXT_SIZE];
    int answer_index;   // 0-3
    int difficulty;     // 1: Easy, 2: Medium, 3: Hard
    char category[100];
} Question;


typedef struct {
    SOCKET socket;
    char username[50];
    int is_logged_in;
    int score;
    int is_busy;
    char pending_invite_from[50];
    char current_opponent[50];
    int game_session_id;
    int current_question_index;
    int last_lobby_version;
    int opponent_quit; // Flag để đánh dấu đối thủ đã thoát
    int last_chat_version; // Version cuối cùng client đã nhận
} ClientState;

typedef struct {
    int id;
    long long game_key;
    char player1[50];
    char player2[50]; // Nếu rỗng -> Chế độ chơi đơn (Classic)
    int score1;
    int score2;
    int total_questions;
    int is_active;
    int used_question_ids[MAX_QUESTIONS_PER_GAME];
    
    // Tracking
    int player1_answers[MAX_QUESTIONS_PER_GAME];
    int player2_answers[MAX_QUESTIONS_PER_GAME];
    double player1_times[MAX_QUESTIONS_PER_GAME];
    double player2_times[MAX_QUESTIONS_PER_GAME];

    // Lifelines: 0=Chưa dùng, 1=Đã dùng
    // Index: 1=50:50, 2=Audience, 3=Call, 4=Swap
    int p1_lifelines[5]; 
    int p2_lifelines[5]; 
} GameSession;

typedef struct {
    long long game_key;
    char player1[50];
    char player2[50];
    int score1;
    int score2;
    int total_questions;
    time_t finished_time;
} GameHistory;

typedef struct {
    char username[50];
    int score;
} PlayerScore;

#endif