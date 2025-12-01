#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// Include server_state.h đầu tiên vì nó chứa models.h (đã có winsock2.h)
#include "../include/server_state.h" 
#include "../../common/protocol.h"
#include "../../common/cJSON.h"
#include "../include/game_service.h"
#include "../include/data_manager.h"

#define MAX_TIME_PER_QUESTION 120
#define BASE_SCORE 100

const int PRIZE_LADDER[15] = {
    200000, 400000, 600000, 1000000, 2000000,       // Câu 1-5
    3000000, 6000000, 10000000, 14000000, 22000000, // Câu 6-10
    30000000, 40000000, 80000000, 150000000, 250000000 // Câu 11-15
};

// --- CÁC HÀM HELPER ---

int get_current_prize(int q_index) {
    if (q_index < 0 || q_index >= 15) return 0;
    return PRIZE_LADDER[q_index];
}

int get_safe_reward(int current_q_index) {
    // current_q_index là câu đang trả lời và bị sai.
    // Ví dụ: Đang trả lời câu 6 (index 5) mà sai -> Về mốc câu 5 (index 4)
    
    if (current_q_index < 5) return 0; // Chưa qua câu 5 -> 0đ
    if (current_q_index < 10) return PRIZE_LADDER[4]; // Qua câu 5, chưa qua 10 -> 2.000.000
    return PRIZE_LADDER[9]; // Qua câu 10 -> 22.000.000
}

int compare_scores(const void *a, const void *b) {
    PlayerScore *p1 = (PlayerScore *)a;
    PlayerScore *p2 = (PlayerScore *)b;
    return (p2->score - p1->score); // Sắp xếp giảm dần
}

int find_client_index(const char* username) {
    for (int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && strcmp(clients[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

// Hàm nội bộ: Lấy ID câu hỏi ngẫu nhiên từ RAM theo độ khó
// level: 1 (Dễ), 2 (TB), 3 (Khó)
int get_random_question_id_by_level(int level, int *used_ids, int used_count) {
    int candidates[MAX_QUESTIONS];
    int count = 0;

    for (int i = 0; i < question_count; i++) {
        if (question_bank[i].difficulty == level) {
            // Kiểm tra trùng
            int is_used = 0;
            for (int j = 0; j < used_count; j++) {
                if (used_ids[j] == question_bank[i].id) {
                    is_used = 1;
                    break;
                }
            }
            
            if (!is_used) {
                candidates[count++] = question_bank[i].id;
            }
        }
    }

    if (count == 0) return -1; // Hết câu hỏi loại này
    
    int rand_idx = rand() % count;
    return candidates[rand_idx];
}

// --- CÁC HÀM LOGIC GAME CHÍNH ---

int create_game_session(const char* p1, const char* p2, int num_questions) {
    EnterCriticalSection(&cs_games);
    for (int i = 0; i < MAX_GAME_SESSIONS; i++) {
        if (!game_sessions[i].is_active) {
            // Reset toàn bộ session
            memset(&game_sessions[i], 0, sizeof(GameSession));
            
            // Tạo Game Key unique dựa trên thời gian
            long long game_key = (long long)GetTickCount64() + (long long)time(NULL) * 1000000LL + (i * 1000);
            Sleep(1); // Delay nhỏ để tránh trùng key nếu tạo quá nhanh
            
            game_sessions[i].id = i;
            game_sessions[i].game_key = game_key;
            strcpy(game_sessions[i].player1, p1);
            strcpy(game_sessions[i].player2, p2);
            game_sessions[i].total_questions = num_questions;
            game_sessions[i].is_active = 1;
            
            // Reset mảng câu hỏi và câu trả lời
            for (int k = 0; k < MAX_QUESTIONS_PER_GAME; k++) {
                game_sessions[i].used_question_ids[k] = -1;
                game_sessions[i].player1_answers[k] = -1;
                game_sessions[i].player2_answers[k] = -1;
            }
            
            // Reset quyền trợ giúp (0: chưa dùng)
            // Index 1-4 tương ứng với LIFELINE_ID
            memset(game_sessions[i].p1_lifelines, 0, sizeof(game_sessions[i].p1_lifelines));
            memset(game_sessions[i].p2_lifelines, 0, sizeof(game_sessions[i].p2_lifelines));
            
            unsigned int game_seed = (unsigned int)time(NULL) + i * 1000 + (unsigned int)(GetTickCount() % 10000);
            srand(game_seed);
            
            // === TẠO BỘ CÂU HỎI THEO ĐỘ KHÓ ===
            // Chiến thuật: Chia 3 phần (Dễ -> TB -> Khó)
            for (int j = 0; j < num_questions; j++) {
                int level = 1; 
                
                if (num_questions >= 10) {
                    // Nếu >= 10 câu: 30% Dễ, 40% TB, 30% Khó
                    if (j >= num_questions * 0.3) level = 2;
                    if (j >= num_questions * 0.7) level = 3;
                } else {
                    // Nếu ít câu (5 câu): 2 Dễ, 2 TB, 1 Khó
                    if (j >= 2) level = 2;
                    if (j >= 4) level = 3;
                }

                int qid = get_random_question_id_by_level(level, game_sessions[i].used_question_ids, j);
                
                // Fallback: Nếu hết câu khó thì lấy câu trung bình, hết TB thì lấy Dễ
                if (qid == -1) qid = get_random_question_id_by_level(2, game_sessions[i].used_question_ids, j);
                if (qid == -1) qid = get_random_question_id_by_level(1, game_sessions[i].used_question_ids, j);
                
                // Fallback cuối cùng: Lấy đại câu chưa dùng
                if (qid == -1) {
                     for(int z=0; z<question_count; z++) {
                         int used = 0;
                         for(int m=0; m<j; m++) if(game_sessions[i].used_question_ids[m] == question_bank[z].id) used=1;
                         if(!used) { qid = question_bank[z].id; break; }
                     }
                }

                game_sessions[i].used_question_ids[j] = qid;
                // Debug log
                // printf("[Game] Session %d Q%d: ID=%d (Lv %d)\n", i, j+1, qid, level);
            }
            
            LeaveCriticalSection(&cs_games);
            return i;
        }
    }
    LeaveCriticalSection(&cs_games);
    return -1;
}

int calculate_score(int is_correct, double time_taken) {
    if (!is_correct) return 0;
    
    // Công thức: 100 điểm trừ dần theo thời gian
    // Trả lời càng nhanh = điểm càng cao (tối đa 100, tối thiểu 10)
    double time_factor = 1.0 - (time_taken / (double)MAX_TIME_PER_QUESTION);
    
    // Đảm bảo factor trong khoảng [0.1, 1.0]
    if (time_factor > 1.0) time_factor = 1.0;
    if (time_factor < 0.1) time_factor = 0.1;
    
    return (int)(BASE_SCORE * time_factor);
}

// === XỬ LÝ QUYỀN TRỢ GIÚP (LIFELINES) ===
cJSON* process_lifeline(int game_id, const char* username, int lifeline_id) {
    cJSON *res_data = cJSON_CreateObject();
    
    if (game_id < 0 || !game_sessions[game_id].is_active) {
        cJSON_AddStringToObject(res_data, "status", "ERROR");
        return res_data;
    }

    GameSession *gs = &game_sessions[game_id];
    int is_p1 = (strcmp(gs->player1, username) == 0);
    int *lifelines = is_p1 ? gs->p1_lifelines : gs->p2_lifelines;

    // 1. Kiểm tra đã dùng chưa
    if (lifelines[lifeline_id] == 1) {
        cJSON_AddStringToObject(res_data, "status", "ALREADY_USED");
        return res_data;
    }

    // 2. Lấy câu hỏi hiện tại của người chơi
    int c_idx = find_client_index(username);
    if (c_idx == -1) return NULL;
    
    int q_idx = clients[c_idx].current_question_index;
    // Kiểm tra index hợp lệ
    if (q_idx >= gs->total_questions) return NULL;
    
    int q_id = gs->used_question_ids[q_idx];
    
    // Tìm câu hỏi trong RAM
    Question *q = NULL;
    for(int i=0; i<question_count; i++) {
        if (question_bank[i].id == q_id) { q = &question_bank[i]; break; }
    }
    
    if (!q) {
        cJSON_AddStringToObject(res_data, "status", "ERROR_DATA");
        return res_data;
    }

    // 3. Đánh dấu đã dùng
    lifelines[lifeline_id] = 1;
    cJSON_AddStringToObject(res_data, "status", "OK");
    cJSON_AddNumberToObject(res_data, "lifeline_id", lifeline_id);

    // 4. Logic từng quyền
    if (lifeline_id == LIFELINE_5050) {
        // Loại bỏ 2 phương án sai
        int correct = q->answer_index;
        int remove1, remove2;
        
        // Random remove1 khác correct
        do { remove1 = rand() % 4; } while (remove1 == correct);
        // Random remove2 khác correct và remove1
        do { remove2 = rand() % 4; } while (remove2 == correct || remove2 == remove1);
        
        cJSON *removed = cJSON_CreateArray();
        cJSON_AddItemToArray(removed, cJSON_CreateNumber(remove1));
        cJSON_AddItemToArray(removed, cJSON_CreateNumber(remove2));
        cJSON_AddItemToObject(res_data, "removed_indexes", removed);
    }
    else if (lifeline_id == LIFELINE_AUDIENCE) {
        // Giả lập khán giả: Tỷ lệ đúng giảm dần theo độ khó
        int correct = q->answer_index;
        int stats[4] = {0};
        int remain = 100;
        
        // Độ khó càng cao, khán giả càng dễ sai
        int confidence = (q->difficulty == 1) ? 80 : (q->difficulty == 2) ? 55 : 35;
        // Random dao động +/- 10%
        int actual_correct_percent = confidence + (rand() % 20 - 10);
        if (actual_correct_percent > 95) actual_correct_percent = 95;
        if (actual_correct_percent < 20) actual_correct_percent = 20;

        stats[correct] = actual_correct_percent;
        remain -= stats[correct];
        
        // Chia phần còn lại cho 3 đáp án kia
        for (int i=0; i<4; i++) {
            if (i == correct) continue;
            if (remain <= 0) { stats[i] = 0; continue; }
            
            int val = rand() % (remain + 1);
            // Nếu là phần tử sai cuối cùng thì gán nốt phần còn lại
            int is_last_wrong = 1;
            for(int k=i+1; k<4; k++) if(k!=correct) is_last_wrong=0;
            
            if(is_last_wrong) val = remain;
            
            stats[i] = val;
            remain -= val;
        }
        
        cJSON *arr = cJSON_CreateIntArray(stats, 4);
        cJSON_AddItemToObject(res_data, "percentages", arr);
    }
    else if (lifeline_id == LIFELINE_CALL) {
        // Gọi điện thoại: Chuyên gia rởm
        int suggest = q->answer_index;
        // Nếu câu khó, có 30% khả năng chuyên gia chỉ sai
        if (q->difficulty == 3 && (rand() % 100 < 30)) {
             do { suggest = rand() % 4; } while (suggest == q->answer_index);
        }
        
        char *names[] = {"Bo me", "Giao su Xoay", "Google", "Ban than"};
        cJSON_AddNumberToObject(res_data, "suggested_index", suggest);
        cJSON_AddStringToObject(res_data, "friend_name", names[rand() % 4]);
    }
    else if (lifeline_id == LIFELINE_SWAP) {
        // Đổi câu hỏi khác cùng độ khó
        int new_id = get_random_question_id_by_level(q->difficulty, gs->used_question_ids, gs->total_questions);
        // Nếu không tìm được thì lấy đại
        if (new_id == -1) new_id = get_random_question_id_by_level(1, gs->used_question_ids, gs->total_questions);
        
        if (new_id != -1) {
            // Cập nhật vào Session để server biết câu hỏi hiện tại của user đã đổi
            gs->used_question_ids[q_idx] = new_id;
            
            // Lấy data câu hỏi mới trả về
            Question *new_q = NULL;
            for(int i=0; i<question_count; i++) {
                if (question_bank[i].id == new_id) { new_q = &question_bank[i]; break; }
            }
            
            if (new_q) {
                cJSON_AddStringToObject(res_data, "new_question", new_q->question);
                cJSON *opts = cJSON_CreateArray();
                for(int i=0; i<4; i++) cJSON_AddItemToArray(opts, cJSON_CreateString(new_q->options[i]));
                cJSON_AddItemToObject(res_data, "new_options", opts);
                cJSON_AddNumberToObject(res_data, "new_id", new_q->id);
            }
        }
    }

    return res_data;
}

// --- BROADCAST & LEADERBOARD (Dùng cho connection_handler) ---

void broadcast_lobby_update() {
    // Logic đã được tích hợp trong connection_handler thông qua POLL
    // Hàm này giữ lại nếu muốn chủ động gửi (server push)
    // Hiện tại code client dùng POLL nên hàm này có thể để trống hoặc dùng để debug log
}

cJSON* get_leaderboard_json() {
    char *file_content = read_file(ACCOUNT_FILE);
    if (!file_content) return cJSON_CreateArray();

    cJSON *json = cJSON_Parse(file_content);
    if (!json || !cJSON_IsArray(json)) { 
        if(file_content) free(file_content); 
        return cJSON_CreateArray(); 
    }

    int count = cJSON_GetArraySize(json);
    // Giới hạn bộ nhớ alloc
    if (count > 100) count = 100; 
    
    PlayerScore *list = (PlayerScore*)malloc(sizeof(PlayerScore) * count);
    int idx = 0;
    cJSON *acc = NULL;
    cJSON_ArrayForEach(acc, json) {
        if (idx >= count) break;
        cJSON *u = cJSON_GetObjectItem(acc, "username");
        cJSON *s = cJSON_GetObjectItem(acc, "score");
        if (u && s) {
            strncpy(list[idx].username, u->valuestring, 49);
            list[idx].score = s->valueint;
            idx++;
        }
    }
    int valid_count = idx;

    qsort(list, valid_count, sizeof(PlayerScore), compare_scores);

    cJSON *res_arr = cJSON_CreateArray();
    int limit = (valid_count < 20) ? valid_count : 20;
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
