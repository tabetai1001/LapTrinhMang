// src/server/include/server_state.h
#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include "models.h"

// Khai báo biến toàn cục
extern ClientState clients[MAX_CLIENTS];
extern GameSession game_sessions[MAX_GAME_SESSIONS];
extern GameHistory game_history[MAX_HISTORY];
extern int history_count;
extern int lobby_version;

// NGÂN HÀNG CÂU HỎI TRONG RAM
extern Question question_bank[MAX_QUESTIONS];
extern int question_count;

// CHAT MESSAGES
#define MAX_CHAT_MESSAGES 100
typedef struct {
    char username[50];
    char message[256];
    time_t timestamp;
} ChatMessage;
extern ChatMessage chat_messages[MAX_CHAT_MESSAGES];
extern int chat_count;
extern int chat_version;

// Critical Sections
extern CRITICAL_SECTION cs_clients;
extern CRITICAL_SECTION cs_games;
extern CRITICAL_SECTION cs_history;
extern CRITICAL_SECTION cs_lobby;
extern CRITICAL_SECTION cs_data; // Mới: Khóa để ghi file account an toàn
extern CRITICAL_SECTION cs_chat;

void init_server_state();

#endif