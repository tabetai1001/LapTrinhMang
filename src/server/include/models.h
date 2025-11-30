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

// ... (Giữ nguyên phần còn lại của file Models)
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
} ClientState;

typedef struct {
    int id;
    long long game_key;
    char player1[50];
    char player2[50];
    int score1;
    int score2;
    int total_questions;
    int is_active;
    int used_question_ids[MAX_QUESTIONS_PER_GAME];
    int player1_answers[MAX_QUESTIONS_PER_GAME];
    int player2_answers[MAX_QUESTIONS_PER_GAME];
    double player1_times[MAX_QUESTIONS_PER_GAME];
    double player2_times[MAX_QUESTIONS_PER_GAME];
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