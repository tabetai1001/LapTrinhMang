// src/server/modules/data_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../common/cJSON.h" // Sửa đường dẫn include cJSON cho đúng
#include "../include/data_manager.h"
#include "../include/server_state.h"

// Helper: Đọc toàn bộ file vào chuỗi
char* read_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    
    // Nếu không tìm thấy, thử thêm "../" vào đầu (trường hợp chạy từ thư mục bin)
    if (!f) {
        char alt_path[512];
        snprintf(alt_path, sizeof(alt_path), "../%s", filename);
        f = fopen(alt_path, "rb");
    }
    
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

// === 1. LOAD CÂU HỎI VÀO RAM ===
void load_questions_to_memory() {
    char *content = read_file(QUESTION_FILE);
    if (!content) {
        printf("[Data] Error: Cannot read %s. Make sure file exists.\n", QUESTION_FILE);
        return;
    }

    cJSON *json = cJSON_Parse(content);
    if (!json || !cJSON_IsArray(json)) {
        printf("[Data] Error: questions.json is not a valid JSON array.\n");
        free(content);
        return;
    }

    question_count = 0;
    cJSON *item = NULL;
    
    cJSON_ArrayForEach(item, json) {
        if (question_count >= MAX_QUESTIONS) break;

        Question *q = &question_bank[question_count];
        
        // Parse ID
        cJSON *jId = cJSON_GetObjectItem(item, "id");
        q->id = jId ? jId->valueint : 0;

        // Parse Question Text
        cJSON *jTxt = cJSON_GetObjectItem(item, "question");
        if (jTxt && jTxt->valuestring) {
            strncpy(q->question, jTxt->valuestring, QUESTION_TEXT_SIZE - 1);
        }

        // Parse Options
        cJSON *jOpts = cJSON_GetObjectItem(item, "options");
        if (jOpts && cJSON_IsArray(jOpts)) {
            int opt_idx = 0;
            cJSON *opt = NULL;
            cJSON_ArrayForEach(opt, jOpts) {
                if (opt_idx < 4 && opt->valuestring) {
                    strncpy(q->options[opt_idx], opt->valuestring, OPTION_TEXT_SIZE - 1);
                    opt_idx++;
                }
            }
        }

        // Parse Answer Index
        cJSON *jAns = cJSON_GetObjectItem(item, "answer_index");
        q->answer_index = jAns ? jAns->valueint : 0;

        // Parse Difficulty (Map string "easy"/"medium"/"hard" -> 1/2/3)
        cJSON *jDiff = cJSON_GetObjectItem(item, "difficulty");
        q->difficulty = 1; // Default easy
        if (jDiff && jDiff->valuestring) {
            if (strcmp(jDiff->valuestring, "medium") == 0) q->difficulty = 2;
            else if (strcmp(jDiff->valuestring, "hard") == 0) q->difficulty = 3;
        }

        // Parse Category
        cJSON *jCat = cJSON_GetObjectItem(item, "category");
        if (jCat && jCat->valuestring) {
            strncpy(q->category, jCat->valuestring, 99);
        } else {
            strcpy(q->category, "General");
        }

        question_count++;
    }

    printf("[Data] Loaded %d questions into memory.\n", question_count);
    
    cJSON_Delete(json);
    free(content);
}

// === 2. CẬP NHẬT ĐIỂM SỐ (GHI FILE) ===
int update_user_score(const char* username, int score_to_add) {
    EnterCriticalSection(&cs_data); // Khóa để tránh 2 luồng cùng ghi file

    char *content = read_file(ACCOUNT_FILE);
    cJSON *json = content ? cJSON_Parse(content) : NULL;
    
    if (!json) {
        // Nếu file lỗi hoặc không có, tạo mảng mới (nhưng thực tế login đã check file rồi)
        if (content) free(content);
        LeaveCriticalSection(&cs_data);
        return 0;
    }

    int updated = 0;
    cJSON *user_obj = NULL;
    
    // Tìm user và cộng điểm
    cJSON_ArrayForEach(user_obj, json) {
        cJSON *u = cJSON_GetObjectItem(user_obj, "username");
        if (u && strcmp(u->valuestring, username) == 0) {
            cJSON *s = cJSON_GetObjectItem(user_obj, "score");
            if (s) {
                int new_score = s->valueint + score_to_add;
                cJSON_SetNumberValue(s, new_score);
                updated = 1;
                printf("[Data] Updated score for %s: +%d => %d\n", username, score_to_add, new_score);
            }
            break;
        }
    }

    // Nếu update thành công, ghi lại file
    if (updated) {
        char *new_content = cJSON_Print(json);
        FILE *f = fopen(ACCOUNT_FILE, "w");
        if (f) {
            fprintf(f, "%s", new_content);
            fclose(f);
        }
        free(new_content);
    }

    cJSON_Delete(json);
    if (content) free(content);
    
    LeaveCriticalSection(&cs_data);
    return updated;
}

// ... (Giữ nguyên các hàm load_history, save_history, check_username cũ)
void save_history_to_file() {
    // ... code cũ ...
    // Lưu ý: Có thể bọc thêm EnterCriticalSection(&cs_data) nếu muốn an toàn tuyệt đối
    // Nhưng hàm này đã dùng cs_history để bảo vệ biến RAM rồi. 
    // Tạm thời giữ nguyên logic cũ.
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
    }
    free(json_str);
    cJSON_Delete(history_arr);
}

void load_history_from_file() {
    // ... code cũ ... (copy lại từ server.c cũ hoặc data_manager.c cũ)
     char *file_content = read_file(HISTORY_FILE);
    if (!file_content) return;
    
    cJSON *history_arr = cJSON_Parse(file_content);
    if (!history_arr || !cJSON_IsArray(history_arr)) {
        free(file_content);
        return;
    }
    
    EnterCriticalSection(&cs_history);
    history_count = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, history_arr) {
        if (history_count >= MAX_HISTORY) break;
        cJSON *gk = cJSON_GetObjectItem(item, "game_key");
        cJSON *p1 = cJSON_GetObjectItem(item, "player1");
        cJSON *p2 = cJSON_GetObjectItem(item, "player2");
        cJSON *s1 = cJSON_GetObjectItem(item, "score1");
        cJSON *s2 = cJSON_GetObjectItem(item, "score2");
        cJSON *tq = cJSON_GetObjectItem(item, "total_questions");
        cJSON *tm = cJSON_GetObjectItem(item, "timestamp");
        
        if (gk && p1 && p2 && s1 && s2 && tq && tm) {
            game_history[history_count].game_key = (long long)gk->valuedouble;
            strcpy(game_history[history_count].player1, p1->valuestring);
            strcpy(game_history[history_count].player2, p2->valuestring);
            game_history[history_count].score1 = s1->valueint;
            game_history[history_count].score2 = s2->valueint;
            game_history[history_count].total_questions = tq->valueint;
            game_history[history_count].finished_time = (time_t)tm->valuedouble;
            history_count++;
        }
    }
    LeaveCriticalSection(&cs_history);
    cJSON_Delete(history_arr);
    free(file_content);
}

int check_username_exists(const char* username) {
    // ... code cũ ...
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