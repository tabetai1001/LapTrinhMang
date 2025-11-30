#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "protocol.h"
#include "../include/game_service.h"
#include "../include/server_state.h"
#include "../include/data_manager.h"

#define MAX_TIME_PER_QUESTION 15
#define BASE_SCORE 100

int compare_scores(const void *a, const void *b) {
    PlayerScore *p1 = (PlayerScore *)a;
    PlayerScore *p2 = (PlayerScore *)b;
    return (p2->score - p1->score); // Giảm dần
}

int find_client_index(const char* username) {
    for (int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && strcmp(clients[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

void broadcast_lobby_update() {
    printf("[Game] Broadcasting lobby update to all online clients\n");
    
    char *file_content = read_file(ACCOUNT_FILE);
    if (!file_content) return;
    
    cJSON *accounts = cJSON_Parse(file_content);
    if (!accounts || !cJSON_IsArray(accounts)) {
        if (accounts) cJSON_Delete(accounts);
        free(file_content);
        return;
    }
    
    EnterCriticalSection(&cs_clients);
    
    for (int client_idx = 0; client_idx < MAX_CLIENTS; client_idx++) {
        if (clients[client_idx].socket == 0 || !clients[client_idx].is_logged_in) {
            continue;
        }
        
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "type", MSG_TYPE_LOBBY_LIST);
        cJSON *arr = cJSON_CreateArray();
        
        cJSON *acc = NULL;
        cJSON_ArrayForEach(acc, accounts) {
            cJSON *username_json = cJSON_GetObjectItem(acc, "username");
            if (username_json) {
                char *username = username_json->valuestring;
                
                if (strcmp(username, clients[client_idx].username) == 0) continue;
                
                int is_online = 0;
                int is_in_game = 0;
                
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket != 0 && clients[i].is_logged_in && 
                        strcmp(clients[i].username, username) == 0) {
                        is_online = 1;
                        is_in_game = clients[i].is_busy;
                        break;
                    }
                }
                
                cJSON *player_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(player_obj, "name", username);
                
                if (!is_online) cJSON_AddStringToObject(player_obj, "status", "OFFLINE");
                else if (is_in_game) cJSON_AddStringToObject(player_obj, "status", "IN_GAME");
                else cJSON_AddStringToObject(player_obj, "status", "FREE");
                
                cJSON_AddItemToArray(arr, player_obj);
            }
        }
        
        cJSON_AddItemToObject(msg, "players", arr);
        
        char *msg_str = cJSON_PrintUnformatted(msg);
        send(clients[client_idx].socket, msg_str, strlen(msg_str), 0);
        free(msg_str);
        cJSON_Delete(msg);
    }
    
    LeaveCriticalSection(&cs_clients);
    cJSON_Delete(accounts);
    free(file_content);
}

int create_game_session(const char* p1, const char* p2, int num_questions) {
    EnterCriticalSection(&cs_games);
    for (int i = 0; i < MAX_GAME_SESSIONS; i++) {
        if (!game_sessions[i].is_active) {
            memset(&game_sessions[i], 0, sizeof(GameSession));
            
            long long game_key = (long long)GetTickCount64() + (long long)time(NULL) * 1000000LL + (i * 1000);
            Sleep(1);
            
            game_sessions[i].id = i;
            game_sessions[i].game_key = game_key;
            strcpy(game_sessions[i].player1, p1);
            strcpy(game_sessions[i].player2, p2);
            game_sessions[i].total_questions = num_questions;
            game_sessions[i].is_active = 1;
            
            for (int j = 0; j < MAX_QUESTIONS_PER_GAME; j++) {
                game_sessions[i].used_question_ids[j] = -1;
                game_sessions[i].player1_answers[j] = -1;
                game_sessions[i].player2_answers[j] = -1;
            }
            
            unsigned int game_seed = (unsigned int)time(NULL) + i * 1000 + (unsigned int)(GetTickCount() % 10000);
            srand(game_seed);
            
            for (int j = 0; j < num_questions; j++) {
                cJSON *question = get_random_question(game_sessions[i].used_question_ids, j);
                if (question) {
                    int qid = cJSON_GetObjectItem(question, "id")->valueint;
                    game_sessions[i].used_question_ids[j] = qid;
                    cJSON_Delete(question);
                }
            }
            
            LeaveCriticalSection(&cs_games);
            return i;
        }
    }
    LeaveCriticalSection(&cs_games);
    return -1;
}

cJSON* get_random_question(int *used_ids, int used_count) {
    char *file_content = read_file(QUESTION_FILE);
    if (!file_content) return NULL;
    
    cJSON *json = cJSON_Parse(file_content);
    if (!json || !cJSON_IsArray(json)) {
        free(file_content);
        return NULL;
    }
    
    int total = cJSON_GetArraySize(json);
    if (total == 0) {
        cJSON_Delete(json);
        free(file_content);
        return NULL;
    }
    
    int attempts = 0;
    while (attempts < 100) {
        int random_idx = rand() % total;
        cJSON *q = cJSON_GetArrayItem(json, random_idx);
        int qid = cJSON_GetObjectItem(q, "id")->valueint;
        
        int already_used = 0;
        for (int i = 0; i < used_count; i++) {
            if (used_ids[i] == qid) {
                already_used = 1;
                break;
            }
        }
        
        if (!already_used) {
            cJSON *result = cJSON_Duplicate(q, 1);
            cJSON_Delete(json);
            free(file_content);
            return result;
        }
        attempts++;
    }
    
    cJSON_Delete(json);
    free(file_content);
    return NULL;
}

int calculate_score(int is_correct, double time_taken) {
    if (!is_correct) return 0;
    double time_factor = 1.0 - (time_taken / MAX_TIME_PER_QUESTION);
    if (time_factor < 0.1) time_factor = 0.1;
    return (int)(BASE_SCORE * time_factor);
}

cJSON* get_leaderboard_json() {
    char *file_content = read_file(ACCOUNT_FILE);
    if (!file_content) return cJSON_CreateArray();

    cJSON *json = cJSON_Parse(file_content);
    if (!json || !cJSON_IsArray(json)) { free(file_content); return cJSON_CreateArray(); }

    int count = cJSON_GetArraySize(json);
    PlayerScore *list = (PlayerScore*)malloc(sizeof(PlayerScore) * count);
    int idx = 0;
    cJSON *acc = NULL;
    cJSON_ArrayForEach(acc, json) {
        strcpy(list[idx].username, cJSON_GetObjectItem(acc, "username")->valuestring);
        list[idx].score = cJSON_GetObjectItem(acc, "score")->valueint;
        idx++;
    }

    qsort(list, count, sizeof(PlayerScore), compare_scores);

    cJSON *res_arr = cJSON_CreateArray();
    int limit = (count < 20) ? count : 20;
    for (int i = 0; i < limit; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", list[i].username);
        cJSON_AddNumberToObject(item, "score", list[i].score);
        cJSON_AddItemToArray(res_arr, item);
    }
    free(list);
    cJSON_Delete(json);
    free(file_content);
    return res_arr;
}