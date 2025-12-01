// src/server/modules/server_state.c
#include <stdio.h>
#include "../include/server_state.h"

ClientState clients[MAX_CLIENTS];
GameSession game_sessions[MAX_GAME_SESSIONS];
GameHistory game_history[MAX_HISTORY];
int history_count = 0;
int lobby_version = 0;

// Dữ liệu câu hỏi
Question question_bank[MAX_QUESTIONS];
int question_count = 0;

CRITICAL_SECTION cs_clients;
CRITICAL_SECTION cs_games;
CRITICAL_SECTION cs_history;
CRITICAL_SECTION cs_lobby;
CRITICAL_SECTION cs_data; // Mới

void init_server_state() {
    InitializeCriticalSection(&cs_clients);
    InitializeCriticalSection(&cs_games);
    InitializeCriticalSection(&cs_history);
    InitializeCriticalSection(&cs_lobby);
    InitializeCriticalSection(&cs_data);

    for (int i = 0; i < MAX_GAME_SESSIONS; i++) {
        game_sessions[i].is_active = 0;
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
    }
    
    // Reset question count
    question_count = 0;

    printf("[State] Server state initialized.\n");
}