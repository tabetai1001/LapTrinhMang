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

// Critical Sections
extern CRITICAL_SECTION cs_clients;
extern CRITICAL_SECTION cs_games;
extern CRITICAL_SECTION cs_history;
extern CRITICAL_SECTION cs_lobby;
extern CRITICAL_SECTION cs_data; // Mới: Khóa để ghi file account an toàn

void init_server_state();

#endif