#include <stdio.h>
#include "../include/server_state.h"

// Định nghĩa thực sự các biến toàn cục
ClientState clients[MAX_CLIENTS];
GameSession game_sessions[MAX_GAME_SESSIONS];
GameHistory game_history[MAX_HISTORY];
int history_count = 0;
int lobby_version = 0;

CRITICAL_SECTION cs_clients;
CRITICAL_SECTION cs_games;
CRITICAL_SECTION cs_history;
CRITICAL_SECTION cs_lobby;

void init_server_state() {
    InitializeCriticalSection(&cs_clients);
    InitializeCriticalSection(&cs_games);
    InitializeCriticalSection(&cs_history);
    InitializeCriticalSection(&cs_lobby);

    // Reset game sessions
    for (int i = 0; i < MAX_GAME_SESSIONS; i++) {
        game_sessions[i].is_active = 0;
    }
    
    // Reset clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
    }

    printf("[State] Server state initialized.\n");
}