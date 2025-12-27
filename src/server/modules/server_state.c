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

// Chat messages
ChatMessage chat_messages[MAX_CHAT_MESSAGES];
int chat_count = 0;
int chat_version = 0;

pthread_mutex_t cs_clients = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cs_games = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cs_history = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cs_lobby = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cs_data = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cs_chat = PTHREAD_MUTEX_INITIALIZER;

void init_server_state() {
    pthread_mutex_init(&cs_clients, NULL);
    pthread_mutex_init(&cs_games, NULL);
    pthread_mutex_init(&cs_history, NULL);
    pthread_mutex_init(&cs_lobby, NULL);
    pthread_mutex_init(&cs_data, NULL);
    pthread_mutex_init(&cs_chat, NULL);

    for (int i = 0; i < MAX_GAME_SESSIONS; i++) {
        game_sessions[i].is_active = 0;
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
    }
    
    // Reset question count
    question_count = 0;
    
    // Reset chat
    chat_count = 0;
    chat_version = 0;

    printf("[State] Server state initialized.\n");
}