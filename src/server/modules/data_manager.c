#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../include/data_manager.h"
#include "../include/server_state.h"

char* read_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char*)malloc(length + 1);
    if (data) {
        fread(data, 1, length, f);
        data[length] = '\0';
    }
    fclose(f);
    return data;
}

void save_history_to_file() {
    cJSON *history_arr = cJSON_CreateArray();
    
    EnterCriticalSection(&cs_history);
    for (int i = 0; i < history_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "game_key", (double)game_history[i].game_key);
        cJSON_AddStringToObject(item, "player1", game_history[i].player1);
        cJSON_AddStringToObject(item, "player2", game_history[i].player2);
        cJSON_AddNumberToObject(item, "score1", game_history[i].score1);
        cJSON_AddNumberToObject(item, "score2", game_history[i].score2);
        cJSON_AddNumberToObject(item, "total_questions", game_history[i].total_questions);
        cJSON_AddNumberToObject(item, "timestamp", (double)game_history[i].finished_time);
        
        cJSON_AddItemToArray(history_arr, item);
    }
    LeaveCriticalSection(&cs_history);
    
    char *json_str = cJSON_Print(history_arr);
    FILE *f = fopen(HISTORY_FILE, "w");
    if (f) {
        fprintf(f, "%s", json_str);
        fclose(f);
        printf("[Data] Saved %d history records to file\n", history_count);
    } else {
        printf("[Data] ERROR: Could not save history to file\n");
    }
    
    free(json_str);
    cJSON_Delete(history_arr);
}

void load_history_from_file() {
    char *file_content = read_file(HISTORY_FILE);
    if (!file_content) {
        printf("[Data] No history file found, starting fresh\n");
        return;
    }
    
    cJSON *history_arr = cJSON_Parse(file_content);
    if (!history_arr || !cJSON_IsArray(history_arr)) {
        printf("[Data] Invalid history file format\n");
        free(file_content);
        return;
    }
    
    EnterCriticalSection(&cs_history);
    history_count = 0;
    
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, history_arr) {
        if (history_count >= MAX_HISTORY) break;
        
        cJSON *game_key = cJSON_GetObjectItem(item, "game_key");
        cJSON *player1 = cJSON_GetObjectItem(item, "player1");
        cJSON *player2 = cJSON_GetObjectItem(item, "player2");
        cJSON *score1 = cJSON_GetObjectItem(item, "score1");
        cJSON *score2 = cJSON_GetObjectItem(item, "score2");
        cJSON *total_q = cJSON_GetObjectItem(item, "total_questions");
        cJSON *timestamp = cJSON_GetObjectItem(item, "timestamp");
        
        if (game_key && player1 && player2 && score1 && score2 && total_q && timestamp) {
            game_history[history_count].game_key = (long long)game_key->valuedouble;
            strcpy(game_history[history_count].player1, player1->valuestring);
            strcpy(game_history[history_count].player2, player2->valuestring);
            game_history[history_count].score1 = score1->valueint;
            game_history[history_count].score2 = score2->valueint;
            game_history[history_count].total_questions = total_q->valueint;
            game_history[history_count].finished_time = (time_t)timestamp->valuedouble;
            history_count++;
        }
    }
    LeaveCriticalSection(&cs_history);
    
    printf("[Data] Loaded %d history records from file\n", history_count);
    
    cJSON_Delete(history_arr);
    free(file_content);
}

int check_username_exists(const char* username) {
    char *file_content = read_file(ACCOUNT_FILE);
    if (!file_content) return 0;
    
    cJSON *json = cJSON_Parse(file_content);
    int exists = 0;
    
    if (json && cJSON_IsArray(json)) {
        cJSON *acc = NULL;
        cJSON_ArrayForEach(acc, json) {
            cJSON *u = cJSON_GetObjectItem(acc, "username");
            if (u && strcmp(u->valuestring, username) == 0) {
                exists = 1;
                break;
            }
        }
    }
    
    if (json) cJSON_Delete(json);
    free(file_content);
    return exists;
}