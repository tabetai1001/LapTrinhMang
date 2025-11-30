#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <windows.h>
#include "models.h"

// Khai báo biến toàn cục (Implementation nằm ở server_state.c)
extern ClientState clients[MAX_CLIENTS];
extern GameSession game_sessions[MAX_GAME_SESSIONS];
extern GameHistory game_history[MAX_HISTORY];
extern int history_count;
extern int lobby_version;

// Critical Sections để xử lý đa luồng an toàn
extern CRITICAL_SECTION cs_clients;
extern CRITICAL_SECTION cs_games;
extern CRITICAL_SECTION cs_history;
extern CRITICAL_SECTION cs_lobby;

// Hàm khởi tạo state
void init_server_state();

#endif