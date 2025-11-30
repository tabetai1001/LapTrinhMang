/*
 * server.c - Full Version (PvP + Leaderboard + Login)
 * Compile: gcc -o server server.c cJSON.c -lws2_32 -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h> 

#include "protocol.h"
#include "cJSON.h"



#define PORT 5555
#define MAX_CLIENTS 30
#define ACCOUNT_FILE "data/accounts.json"
#define QUESTION_FILE "data/questions.json"
#define HISTORY_FILE "data/history.json"
#define MAX_GAME_SESSIONS 10

// Đảm bảo BUFFER_SIZE được định nghĩa
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 4096
#endif

typedef struct {
    SOCKET socket;
    char username[50];
    int is_logged_in;
    int score;
    // PvP State
    int is_busy;              // 0: Rảnh, 1: Đang bận/Đang chơi
    char pending_invite_from[50]; // Lưu tên người đang mời mình
    char current_opponent[50];    // Lưu tên đối thủ hiện tại
    int game_session_id;      // ID của phiên game hiện tại (-1 nếu không chơi)
    int current_question_index; // Câu hỏi hiện tại của người chơi này
    int last_lobby_version;   // Version lobby mà client đã biết
} ClientState;

typedef struct {
    int id;
    long long game_key;  // KEY DUY NHẤT cho mỗi trận đấu (timestamp)
    char player1[50];
    char player2[50];
    int score1;
    int score2;
    int total_questions;
    int is_active;
    int used_question_ids[MAX_QUESTIONS_PER_GAME]; // Bộ câu hỏi chung cho cả 2
    // Lưu lịch sử trả lời
    int player1_answers[MAX_QUESTIONS_PER_GAME];
    int player2_answers[MAX_QUESTIONS_PER_GAME];
    double player1_times[MAX_QUESTIONS_PER_GAME];
    double player2_times[MAX_QUESTIONS_PER_GAME];
} GameSession;

// Lịch sử các trận đấu đã hoàn thành
#define MAX_HISTORY 100
typedef struct {
    long long game_key;
    char player1[50];
    char player2[50];
    int score1;
    int score2;
    int total_questions;
    time_t finished_time;
} GameHistory;

GameHistory game_history[MAX_HISTORY];
int history_count = 0;
CRITICAL_SECTION cs_history;

typedef struct {
    char username[50];
    int score;
} PlayerScore;

ClientState clients[MAX_CLIENTS];
GameSession game_sessions[MAX_GAME_SESSIONS];
CRITICAL_SECTION cs_clients;
CRITICAL_SECTION cs_games;
int lobby_version = 0; // Tăng mỗi khi có thay đổi lobby
CRITICAL_SECTION cs_lobby;

// --- CÁC HÀM TIỆN ÍCH ---

char* read_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char*)malloc(length + 1);
    fread(data, 1, length, f);
    fclose(f);
    data[length] = '\0';
    return data;
}

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

// Forward declaration
cJSON* get_random_question(int *used_ids, int used_count);

// Hàm broadcast thông báo cập nhật lobby cho tất cả client đang online
void broadcast_lobby_update() {
    printf("[Server] Broadcasting lobby update to all online clients\n");
    
    // Đọc danh sách tất cả accounts từ file
    char *file_content = read_file(ACCOUNT_FILE);
    if (!file_content) return;
    
    cJSON *accounts = cJSON_Parse(file_content);
    if (!accounts || !cJSON_IsArray(accounts)) {
        if (accounts) cJSON_Delete(accounts);
        free(file_content);
        return;
    }
    
    EnterCriticalSection(&cs_clients);
    
    // Duyệt qua tất cả client đang online
    for (int client_idx = 0; client_idx < MAX_CLIENTS; client_idx++) {
        if (clients[client_idx].socket == 0 || !clients[client_idx].is_logged_in) {
            continue;
        }
        
        // Tạo message LOBBY_LIST với tất cả người chơi
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "type", MSG_TYPE_LOBBY_LIST);
        cJSON *arr = cJSON_CreateArray();
        
        // Duyệt qua tất cả accounts
        cJSON *acc = NULL;
        cJSON_ArrayForEach(acc, accounts) {
            cJSON *username_json = cJSON_GetObjectItem(acc, "username");
            if (username_json) {
                char *username = username_json->valuestring;
                
                // Bỏ qua chính client đang nhận
                if (strcmp(username, clients[client_idx].username) == 0) {
                    continue;
                }
                
                // Tìm xem user này có đang online không
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
                
                // Tạo object với thông tin đầy đủ
                cJSON *player_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(player_obj, "name", username);
                
                // Xác định status
                if (!is_online) {
                    cJSON_AddStringToObject(player_obj, "status", "OFFLINE");
                } else if (is_in_game) {
                    cJSON_AddStringToObject(player_obj, "status", "IN_GAME");
                } else {
                    cJSON_AddStringToObject(player_obj, "status", "FREE");
                }
                
                cJSON_AddItemToArray(arr, player_obj);
            }
        }
        
        cJSON_AddItemToObject(msg, "players", arr);
        
        // Gửi message
        char *msg_str = cJSON_PrintUnformatted(msg);
        send(clients[client_idx].socket, msg_str, strlen(msg_str), 0);
        free(msg_str);
        cJSON_Delete(msg);
    }
    
    LeaveCriticalSection(&cs_clients);
    
    cJSON_Delete(accounts);
    free(file_content);
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
        printf("[Server] Saved %d history records to file\n", history_count);
    } else {
        printf("[Server] ERROR: Could not save history to file\n");
    }
    
    free(json_str);
    cJSON_Delete(history_arr);
}

void load_history_from_file() {
    char *file_content = read_file(HISTORY_FILE);
    if (!file_content) {
        printf("[Server] No history file found, starting fresh\n");
        return;
    }
    
    cJSON *history_arr = cJSON_Parse(file_content);
    if (!history_arr || !cJSON_IsArray(history_arr)) {
        printf("[Server] Invalid history file format\n");
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
    
    printf("[Server] Loaded %d history records from file\n", history_count);
    
    cJSON_Delete(history_arr);
    free(file_content);
}

int create_game_session(const char* p1, const char* p2, int num_questions) {
    EnterCriticalSection(&cs_games);
    for (int i = 0; i < MAX_GAME_SESSIONS; i++) {
        if (!game_sessions[i].is_active) {
            printf("[Server] Reusing game session slot %d (was inactive)\n", i);
            
            // RESET HOÀN TOÀN game session để tránh dữ liệu cũ - RẤT QUAN TRỌNG!
            memset(&game_sessions[i], 0, sizeof(GameSession));
            
            // TẠO GAME_KEY DUY NHẤT dựa trên timestamp (milliseconds)
            // Sử dụng GetTickCount64 để có precision tốt hơn và tránh trùng
            long long game_key = (long long)GetTickCount64() + (long long)time(NULL) * 1000000LL + (i * 1000);
            
            // Thêm một chút delay ngẫu nhiên để đảm bảo không trùng
            Sleep(1);  // 1ms delay
            
            game_sessions[i].id = i;
            game_sessions[i].game_key = game_key;
            strcpy(game_sessions[i].player1, p1);
            strcpy(game_sessions[i].player2, p2);
            game_sessions[i].score1 = 0;
            game_sessions[i].score2 = 0;
            game_sessions[i].total_questions = num_questions;
            game_sessions[i].is_active = 1;
            
            printf("[Server] Created game session %d with KEY=%lld\n", i, game_key);
            
            // Khởi tạo used_question_ids về -1
            for (int j = 0; j < MAX_QUESTIONS_PER_GAME; j++) {
                game_sessions[i].used_question_ids[j] = -1;
                game_sessions[i].player1_answers[j] = -1;
                game_sessions[i].player2_answers[j] = -1;
                game_sessions[i].player1_times[j] = 0;
                game_sessions[i].player2_times[j] = 0;
            }
            
            // Thêm entropy để mỗi game session có random seed khác nhau
            unsigned int game_seed = (unsigned int)time(NULL) + i * 1000 + (unsigned int)(GetTickCount() % 10000);
            srand(game_seed);
            printf("[Server] Game %d using seed: %u\n", i, game_seed);
            
            // Tạo bộ câu hỏi MỚI cho cả 2 người
            for (int j = 0; j < num_questions; j++) {
                cJSON *question = get_random_question(game_sessions[i].used_question_ids, j);
                if (question) {
                    int qid = cJSON_GetObjectItem(question, "id")->valueint;
                    game_sessions[i].used_question_ids[j] = qid;
                    printf("[Server] Game %d Q%d: ID=%d\n", i, j+1, qid);
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
    
    // Tìm câu hỏi chưa dùng
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
    
    // Công thức: BASE_SCORE * (1 - time_taken / MAX_TIME)
    // Trả lời nhanh được nhiều điểm hơn
    double time_factor = 1.0 - (time_taken / MAX_TIME_PER_QUESTION);
    if (time_factor < 0.1) time_factor = 0.1; // Tối thiểu 10%
    
    int score = (int)(BASE_SCORE * time_factor);
    return score;
}

// --- LOGIC XỬ LÝ DỮ LIỆU ---

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

int process_register(const char* user, const char* pass) {
    // Kiểm tra username đã tồn tại chưa
    if (check_username_exists(user)) {
        return 0; // Username đã tồn tại
    }
    
    // Đọc file accounts hiện tại
    char *file_content = read_file(ACCOUNT_FILE);
    cJSON *json = NULL;
    
    if (file_content) {
        json = cJSON_Parse(file_content);
        free(file_content);
    }
    
    if (!json || !cJSON_IsArray(json)) {
        json = cJSON_CreateArray(); // Tạo mới nếu file không hợp lệ
    }
    
    // Tạo tài khoản mới
    cJSON *new_account = cJSON_CreateObject();
    cJSON_AddStringToObject(new_account, "username", user);
    cJSON_AddStringToObject(new_account, "password", pass);
    cJSON_AddNumberToObject(new_account, "score", 0); // Điểm khởi đầu là 0
    
    // Thêm vào mảng
    cJSON_AddItemToArray(json, new_account);
    
    // Lưu vào file
    char *json_str = cJSON_Print(json);
    FILE *f = fopen(ACCOUNT_FILE, "w");
    int success = 0;
    
    if (f) {
        fprintf(f, "%s", json_str);
        fclose(f);
        success = 1;
        printf("[Server] New account registered: %s\n", user);
    } else {
        printf("[Server] ERROR: Could not save new account\n");
    }
    
    free(json_str);
    cJSON_Delete(json);
    return success;
}

int process_login(const char* user, const char* pass, int* out_score) {
    char *file_content = read_file(ACCOUNT_FILE);
    if (!file_content) return 0;
    cJSON *json = cJSON_Parse(file_content);
    int success = 0;

    if (json && cJSON_IsArray(json)) {
        cJSON *acc = NULL;
        cJSON_ArrayForEach(acc, json) {
            cJSON *u = cJSON_GetObjectItem(acc, "username");
            cJSON *p = cJSON_GetObjectItem(acc, "password");
            cJSON *s = cJSON_GetObjectItem(acc, "score");
            if (strcmp(u->valuestring, user) == 0 && strcmp(p->valuestring, pass) == 0) {
                success = 1;
                if (out_score) *out_score = s->valueint;
                break;
            }
        }
    }
    if (json) cJSON_Delete(json);
    free(file_content);
    return success;
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

// --- LUỒNG XỬ LÝ CLIENT CHÍNH ---

DWORD WINAPI handle_client(LPVOID client_socket_ptr) {
    SOCKET client_socket = (SOCKET)client_socket_ptr;
    char buffer[BUFFER_SIZE];
    int n, client_index = -1;

    // 1. Đăng ký slot
    EnterCriticalSection(&cs_clients);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = client_socket;
            clients[i].is_logged_in = 0;
            clients[i].is_busy = 0;
            strcpy(clients[i].pending_invite_from, "");
            clients[i].game_session_id = -1;
            clients[i].current_question_index = 0;
            clients[i].last_lobby_version = -1; // Chưa biết version nào
            client_index = i;
            break;
        }
    }
    LeaveCriticalSection(&cs_clients);

    if (client_index == -1) { closesocket(client_socket); return 0; }

    // 2. Vòng lặp nhận tin
    while ((n = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("[Server] %s: %s\n", clients[client_index].is_logged_in ? clients[client_index].username : "Unknown", buffer);

        cJSON *req = cJSON_Parse(buffer);
        if (!req) continue;
        cJSON *type = cJSON_GetObjectItem(req, "type");
        if (!type) { cJSON_Delete(req); continue; }

        cJSON *res = cJSON_CreateObject();

        // --- XỬ LÝ ĐĂNG KÝ ---
        if (strcmp(type->valuestring, MSG_TYPE_REGISTER) == 0) {
            cJSON *u = cJSON_GetObjectItem(req, "user");
            cJSON *p = cJSON_GetObjectItem(req, "pass");
            
            if (!u || !p || strlen(u->valuestring) == 0 || strlen(p->valuestring) == 0) {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_FAIL);
                cJSON_AddStringToObject(res, "message", "Username và password không được để trống!");
            } else if (strlen(u->valuestring) < 3) {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_FAIL);
                cJSON_AddStringToObject(res, "message", "Username phải có ít nhất 3 ký tự!");
            } else if (strlen(p->valuestring) < 3) {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_FAIL);
                cJSON_AddStringToObject(res, "message", "Password phải có ít nhất 3 ký tự!");
            } else if (process_register(u->valuestring, p->valuestring)) {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_SUCCESS);
                cJSON_AddStringToObject(res, "message", "Đăng ký thành công! Vui lòng đăng nhập.");
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_FAIL);
                cJSON_AddStringToObject(res, "message", "Username đã tồn tại!");
            }
        }
        
        // --- XỬ LÝ ĐĂNG NHẬP ---
        else if (strcmp(type->valuestring, MSG_TYPE_LOGIN) == 0) {
            cJSON *u = cJSON_GetObjectItem(req, "user");
            cJSON *p = cJSON_GetObjectItem(req, "pass");
            int score = 0;

            if (process_login(u->valuestring, p->valuestring, &score)) {
                EnterCriticalSection(&cs_clients);
                strcpy(clients[client_index].username, u->valuestring);
                clients[client_index].is_logged_in = 1;
                clients[client_index].score = score;
                clients[client_index].is_busy = 0;
                strcpy(clients[client_index].pending_invite_from, "");
                LeaveCriticalSection(&cs_clients);

                cJSON_AddStringToObject(res, "type", MSG_TYPE_LOGIN_SUCCESS);
                cJSON_AddStringToObject(res, "user", u->valuestring);
                cJSON_AddNumberToObject(res, "total_score", score);
                
                // Tăng lobby version để các client khác biết lobby đã thay đổi
                EnterCriticalSection(&cs_lobby);
                lobby_version++;
                clients[client_index].last_lobby_version = lobby_version; // Client này đã biết version mới
                printf("[Server] User %s logged in, lobby_version=%d\n", u->valuestring, lobby_version);
                LeaveCriticalSection(&cs_lobby);
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_LOGIN_FAIL);
                cJSON_AddStringToObject(res, "message", "Sai tai khoan/mat khau");
            }
        }
        
        // --- XỬ LÝ POLLING (Trái tim của PvP) ---
        else if (strcmp(type->valuestring, MSG_TYPE_POLL) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            if (strlen(clients[client_index].pending_invite_from) > 0) {
                // Parse invite info: "username:num_questions"
                char invite_copy[100];
                strcpy(invite_copy, clients[client_index].pending_invite_from);
                char *colon = strchr(invite_copy, ':');
                int num_q = 5;
                if (colon) {
                    *colon = '\0';
                    num_q = atoi(colon + 1);
                }
                
                cJSON_AddStringToObject(res, "type", MSG_TYPE_RECEIVE_INVITE);
                cJSON_AddStringToObject(res, "from", invite_copy);
                cJSON_AddNumberToObject(res, "num_questions", num_q);
            } 
            else if (clients[client_index].is_busy == 1 && strlen(clients[client_index].current_opponent) > 0 && clients[client_index].game_session_id >= 0) {
                // Logic đơn giản: Nếu bận và có đối thủ và có game session -> Start Game
                int gid = clients[client_index].game_session_id;
                cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_START);
                cJSON_AddStringToObject(res, "opponent", clients[client_index].current_opponent);
                cJSON_AddNumberToObject(res, "total_questions", game_sessions[gid].total_questions);
                cJSON_AddNumberToObject(res, "game_key", (double)game_sessions[gid].game_key);
                printf("[Server] POLL: Sending GAME_START to %s vs %s (%d questions, KEY=%lld)\n", 
                    clients[client_index].username, clients[client_index].current_opponent, game_sessions[gid].total_questions, game_sessions[gid].game_key);
            }
            else {
                // Kiểm tra xem lobby có thay đổi không
                EnterCriticalSection(&cs_lobby);
                int current_lobby_version = lobby_version;
                int client_lobby_version = clients[client_index].last_lobby_version;
                LeaveCriticalSection(&cs_lobby);
                
                printf("[Server] POLL from %s: client_version=%d, current_version=%d\n", 
                       clients[client_index].username, client_lobby_version, current_lobby_version);
                
                if (client_lobby_version != current_lobby_version) {
                    // Lobby đã thay đổi, gửi lobby list mới
                    printf("[Server] POLL: Sending LOBBY_LIST to %s (version %d -> %d)\n", 
                           clients[client_index].username, client_lobby_version, current_lobby_version);
                    
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_LOBBY_LIST);
                    
                    cJSON *arr = cJSON_CreateArray();
                    
                    // Đọc tất cả accounts từ file
                    char *file_content = read_file(ACCOUNT_FILE);
                    if (file_content) {
                        cJSON *accounts = cJSON_Parse(file_content);
                        if (accounts && cJSON_IsArray(accounts)) {
                            cJSON *acc = NULL;
                            cJSON_ArrayForEach(acc, accounts) {
                                cJSON *username_json = cJSON_GetObjectItem(acc, "username");
                                if (username_json) {
                                    char *username = username_json->valuestring;
                                    
                                    // Bỏ qua chính mình
                                    if (strcmp(username, clients[client_index].username) == 0) {
                                        continue;
                                    }
                                    
                                    // Tìm xem user này có đang online không
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
                                    
                                    // Tạo object với thông tin đầy đủ
                                    cJSON *player_obj = cJSON_CreateObject();
                                    cJSON_AddStringToObject(player_obj, "name", username);
                                    
                                    // Xác định status
                                    if (!is_online) {
                                        cJSON_AddStringToObject(player_obj, "status", "OFFLINE");
                                    } else if (is_in_game) {
                                        cJSON_AddStringToObject(player_obj, "status", "IN_GAME");
                                    } else {
                                        cJSON_AddStringToObject(player_obj, "status", "FREE");
                                    }
                                    
                                    cJSON_AddItemToArray(arr, player_obj);
                                }
                            }
                        }
                        if (accounts) cJSON_Delete(accounts);
                        free(file_content);
                    }
                    
                    cJSON_AddItemToObject(res, "players", arr);
                    
                    // Cập nhật version mà client đã biết
                    EnterCriticalSection(&cs_lobby);
                    clients[client_index].last_lobby_version = current_lobby_version;
                    LeaveCriticalSection(&cs_lobby);
                } else {
                    // Không có thay đổi, gửi NO_EVENT
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_NO_EVENT);
                }
            }
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }

        // --- XỬ LÝ MỜI NGƯỜI CHƠI ---
        else if (strcmp(type->valuestring, MSG_TYPE_INVITE_PLAYER) == 0) {
            cJSON *target = cJSON_GetObjectItem(req, "target");
            cJSON *num_q = cJSON_GetObjectItem(req, "num_questions");
            int num_questions = num_q ? num_q->valueint : 5;
            
            EnterCriticalSection(&cs_clients);
            int t_idx = find_client_index(target->valuestring);
            
            if (t_idx != -1 && clients[t_idx].is_busy == 0) {
                strcpy(clients[t_idx].pending_invite_from, clients[client_index].username);
                clients[client_index].is_busy = 1; // Người mời cũng tạm bận chờ
                // Lưu tạm số câu hỏi vào username của người mời (hack nhỏ)
                char invite_info[100];
                sprintf(invite_info, "%s:%d", clients[client_index].username, num_questions);
                strcpy(clients[t_idx].pending_invite_from, invite_info);
                
                cJSON_AddStringToObject(res, "type", "INVITE_SENT_SUCCESS");
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_INVITE_FAIL);
                cJSON_AddStringToObject(res, "message", "Nguoi choi ban hoac offline");
            }
            LeaveCriticalSection(&cs_clients);
        }

        // --- XỬ LÝ CHẤP NHẬN ---
        else if (strcmp(type->valuestring, MSG_TYPE_ACCEPT_INVITE) == 0) {
            cJSON *inviter = cJSON_GetObjectItem(req, "from");
            EnterCriticalSection(&cs_clients);
            int i_idx = find_client_index(inviter->valuestring);
            
            if (i_idx != -1) {
                // Parse số câu hỏi từ pending_invite_from
                char invite_copy[100];
                strcpy(invite_copy, clients[client_index].pending_invite_from);
                char *colon = strchr(invite_copy, ':');
                int num_questions = 5;
                if (colon) {
                    num_questions = atoi(colon + 1);
                }
                
                // Tạo game session với số câu hỏi đã chọn
                int game_id = create_game_session(clients[i_idx].username, clients[client_index].username, num_questions);
                
                if (game_id != -1) {
                    // Thiết lập cặp đấu
                    clients[client_index].is_busy = 1;
                    strcpy(clients[client_index].current_opponent, clients[i_idx].username);
                    strcpy(clients[client_index].pending_invite_from, "");
                    clients[client_index].game_session_id = game_id;
                    clients[client_index].current_question_index = 0;

                    clients[i_idx].is_busy = 1;
                    strcpy(clients[i_idx].current_opponent, clients[client_index].username);
                    clients[i_idx].game_session_id = game_id;
                    clients[i_idx].current_question_index = 0;
                    
                    printf("[Server] Game session %d created: %s vs %s (%d questions, KEY=%lld)\n", 
                        game_id, clients[i_idx].username, clients[client_index].username, num_questions, game_sessions[game_id].game_key);
                    
                    // Tăng lobby_version vì 2 người chơi chuyển sang trạng thái IN_GAME
                    EnterCriticalSection(&cs_lobby);
                    lobby_version++;
                    printf("[Server] Players entering game, lobby_version=%d\n", lobby_version);
                    LeaveCriticalSection(&cs_lobby);
                    
                    // Báo ngay cho người chấp nhận
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_START);
                    cJSON_AddStringToObject(res, "opponent", clients[i_idx].username);
                    cJSON_AddNumberToObject(res, "total_questions", num_questions);
                    cJSON_AddNumberToObject(res, "game_key", (double)game_sessions[game_id].game_key);
                }
            }
            LeaveCriticalSection(&cs_clients);
        }

        // --- XỬ LÝ TỪ CHỐI ---
        else if (strcmp(type->valuestring, MSG_TYPE_REJECT_INVITE) == 0) {
             cJSON *inviter = cJSON_GetObjectItem(req, "from");
             EnterCriticalSection(&cs_clients);
             int i_idx = find_client_index(inviter->valuestring);
             
             strcpy(clients[client_index].pending_invite_from, "");
             if (i_idx != -1) {
                 clients[i_idx].is_busy = 0; // Giải phóng người mời
                 strcpy(clients[i_idx].current_opponent, ""); // Xóa đối thủ
             }
             
             LeaveCriticalSection(&cs_clients);
             cJSON_AddStringToObject(res, "type", "REJECT_SUCCESS");
        }

        // --- XỬ LÝ YÊU CẦU CÂU HỎI ---
        else if (strcmp(type->valuestring, MSG_TYPE_REQUEST_QUESTION) == 0) {
            printf("[Server] %s request question\n", clients[client_index].username);
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            
            int game_id = clients[client_index].game_session_id;
            int my_q_index = clients[client_index].current_question_index;
            
            printf("[Server] %s at Q%d/%d\n", clients[client_index].username, my_q_index + 1, 
                game_id >= 0 ? game_sessions[game_id].total_questions : 0);
            
            if (game_id >= 0 && game_sessions[game_id].is_active) {
                GameSession *gs = &game_sessions[game_id];
                
                if (my_q_index < gs->total_questions) {
                    // Lấy câu hỏi từ bộ câu đã tạo sẵn
                    int qid = gs->used_question_ids[my_q_index];
                    
                    if (qid != -1) {
                        char *file_content = read_file(QUESTION_FILE);
                        cJSON *questions = cJSON_Parse(file_content);
                        cJSON *q = NULL;
                        cJSON *found_question = NULL;
                        
                        cJSON_ArrayForEach(q, questions) {
                            if (cJSON_GetObjectItem(q, "id")->valueint == qid) {
                                found_question = cJSON_Duplicate(q, 1);
                                break;
                            }
                        }
                        
                        if (found_question) {
                            cJSON *q_text = cJSON_GetObjectItem(found_question, "question");
                            cJSON *q_opts = cJSON_GetObjectItem(found_question, "options");
                            int is_p1 = (strcmp(clients[client_index].username, gs->player1) == 0);
                            int opp_score = is_p1 ? gs->score2 : gs->score1;
                            
                            printf("[Server] Sending Q%d/%d (ID:%d) to %s\n", 
                                my_q_index + 1, gs->total_questions, qid, clients[client_index].username);
                            
                            cJSON_AddStringToObject(res, "type", MSG_TYPE_QUESTION);
                            cJSON_AddNumberToObject(res, "question_number", my_q_index + 1);
                            cJSON_AddNumberToObject(res, "total_questions", gs->total_questions);
                            cJSON_AddStringToObject(res, "question", q_text ? q_text->valuestring : "Error");
                            cJSON_AddItemToObject(res, "options", cJSON_Duplicate(q_opts, 1));
                            cJSON_AddNumberToObject(res, "question_id", qid);
                            cJSON_AddNumberToObject(res, "max_time", MAX_TIME_PER_QUESTION);
                            cJSON_AddNumberToObject(res, "opponent_score", opp_score);
                            
                            cJSON_Delete(found_question);
                        }
                        
                        cJSON_Delete(questions);
                        free(file_content);
                    }
                } else {
                    // Người này đã hết câu hỏi
                    cJSON_AddStringToObject(res, "type", "NO_MORE_QUESTIONS");
                    
                    // Kiểm tra xem đối thủ đã xong chưa
                    int opp_idx = find_client_index(clients[client_index].current_opponent);
                    if (opp_idx != -1 && clients[opp_idx].current_question_index >= gs->total_questions) {
                        // Cả 2 đều xong
                        gs->is_active = 0;
                        printf("[Server] Game finished! %s: %d vs %s: %d\n", 
                            gs->player1, gs->score1, gs->player2, gs->score2);
                    }
                }
            } else {
                printf("[Server] ERROR: Invalid game session\n");
                cJSON_AddStringToObject(res, "type", "ERROR");
                cJSON_AddStringToObject(res, "message", "Invalid game session");
            }
            
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        
        // --- XỬ LÝ NỘP ĐÁP ÁN ---
        else if (strcmp(type->valuestring, MSG_TYPE_SUBMIT_ANSWER) == 0) {
            cJSON *qid_json = cJSON_GetObjectItem(req, "question_id");
            cJSON *ans_json = cJSON_GetObjectItem(req, "answer_index");
            cJSON *time_json = cJSON_GetObjectItem(req, "time_taken");
            
            int question_id = qid_json->valueint;
            int answer_index = ans_json->valueint;
            double time_taken = time_json->valuedouble;
            
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            
            int game_id = clients[client_index].game_session_id;
            if (game_id >= 0 && game_sessions[game_id].is_active) {
                GameSession *gs = &game_sessions[game_id];
                
                // Kiểm tra đáp án đúng hay sai
                char *file_content = read_file(QUESTION_FILE);
                cJSON *questions = cJSON_Parse(file_content);
                int is_correct = 0;
                int correct_answer = -1;
                
                cJSON *q = NULL;
                cJSON_ArrayForEach(q, questions) {
                    if (cJSON_GetObjectItem(q, "id")->valueint == question_id) {
                        correct_answer = cJSON_GetObjectItem(q, "answer_index")->valueint;
                        is_correct = (answer_index == correct_answer);
                        break;
                    }
                }
                
                cJSON_Delete(questions);
                free(file_content);
                
                // Tính điểm
                int earned_score = calculate_score(is_correct, time_taken);
                
                // Cập nhật điểm và lưu lịch sử cho người chơi
                int is_player1 = (strcmp(clients[client_index].username, gs->player1) == 0);
                int my_q_index = clients[client_index].current_question_index;
                
                if (is_player1) {
                    gs->score1 += earned_score;
                    gs->player1_answers[my_q_index] = answer_index;
                    gs->player1_times[my_q_index] = time_taken;
                } else {
                    gs->score2 += earned_score;
                    gs->player2_answers[my_q_index] = answer_index;
                    gs->player2_times[my_q_index] = time_taken;
                }
                
                // Tăng câu hỏi của người này
                clients[client_index].current_question_index++;
                
                printf("[Server] %s answered Q%d, earned %d pts, now at Q%d/%d\n", 
                    clients[client_index].username, my_q_index + 1, earned_score, 
                    clients[client_index].current_question_index + 1, gs->total_questions);
                
                // Gửi kết quả
                cJSON_AddStringToObject(res, "type", MSG_TYPE_ANSWER_RESULT);
                cJSON_AddBoolToObject(res, "is_correct", is_correct);
                cJSON_AddNumberToObject(res, "correct_answer", correct_answer);
                cJSON_AddNumberToObject(res, "earned_score", earned_score);
                cJSON_AddNumberToObject(res, "your_total_score", is_player1 ? gs->score1 : gs->score2);
                cJSON_AddNumberToObject(res, "opponent_score", is_player1 ? gs->score2 : gs->score1);
                cJSON_AddNumberToObject(res, "current_question", my_q_index + 1);
                cJSON_AddNumberToObject(res, "total_questions", gs->total_questions);
                
                // Kiểm tra xem người này đã hết câu chưa
                if (clients[client_index].current_question_index >= gs->total_questions) {
                    printf("[Server] %s finished all questions!\n", clients[client_index].username);
                    
                    // Kiểm tra xem đối thủ cũng đã xong chưa
                    int opp_idx = find_client_index(clients[client_index].current_opponent);
                    int opp_finished = (opp_idx != -1 && clients[opp_idx].current_question_index >= gs->total_questions);
                    
                    if (opp_finished) {
                        // Cả 2 đều xong -> Kết thúc game
                        printf("[Server] Both finished! Game over: %s(%d) vs %s(%d)\n", 
                            gs->player1, gs->score1, gs->player2, gs->score2);
                        
                        // LƯU VÀO LỊCH Sử
                        EnterCriticalSection(&cs_history);
                        if (history_count < MAX_HISTORY) {
                            game_history[history_count].game_key = gs->game_key;
                            strcpy(game_history[history_count].player1, gs->player1);
                            strcpy(game_history[history_count].player2, gs->player2);
                            game_history[history_count].score1 = gs->score1;
                            game_history[history_count].score2 = gs->score2;
                            game_history[history_count].total_questions = gs->total_questions;
                            game_history[history_count].finished_time = time(NULL);
                            history_count++;
                            printf("[Server] Saved game KEY=%lld to history (total: %d)\n", gs->game_key, history_count);
                        }
                        LeaveCriticalSection(&cs_history);
                        
                        // LƯU NGAY VÀO FILE
                        save_history_to_file();
                        
                        // Đánh dấu game kết thúc TRƯỚC KHI reset để đối thủ có thể nhận kết quả qua polling
                        gs->is_active = 0;
                        
                        // GỬI THÔNG BÁO KẾT QUẢ CHO NGƯỜI CHƠI HIỆN TẠI
                        cJSON_AddStringToObject(res, "game_status", "FINISHED");
                        cJSON_AddBoolToObject(res, "you_win", 
                            is_player1 ? (gs->score1 > gs->score2) : (gs->score2 > gs->score1));
                        
                        // RESET NGAY người chơi hiện tại (người vừa hoàn thành sau cùng)
                        // để tránh POLL gửi lại GAME_START khi họ về lobby
                        // KHÔNG reset current_question_index để đối thủ có thể kiểm tra
                        clients[client_index].is_busy = 0;
                        clients[client_index].game_session_id = -1;
                        // GIỮ LẠI current_question_index để CHECK_GAME_STATUS có thể kiểm tra
                        // clients[client_index].current_question_index = 0;  // KHÔNG RESET
                        memset(clients[client_index].current_opponent, 0, sizeof(clients[client_index].current_opponent));
                        printf("[Server] Reset %s state (keep question_index=%d) after finishing\n", 
                            clients[client_index].username, clients[client_index].current_question_index);
                        
                        // Người còn lại (đang chờ) sẽ được reset khi họ gọi CHECK_GAME_STATUS
                    } else {
                        // Chỉ mình xong, đối thủ chưa xong
                        cJSON_AddStringToObject(res, "game_status", "WAITING_OPPONENT");
                    }
                } else {
                    // Còn câu hỏi
                    cJSON_AddStringToObject(res, "game_status", "NEXT_QUESTION");
                }
            }
            
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        
        // --- KIỂM TRA TRẠNG THÁI GAME ---
        else if (strcmp(type->valuestring, MSG_TYPE_CHECK_GAME_STATUS) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            
            int gid = clients[client_index].game_session_id;
            if (gid >= 0 && gid < MAX_GAME_SESSIONS) {
                GameSession *gs = &game_sessions[gid];
                int is_player1 = (strcmp(clients[client_index].username, gs->player1) == 0);
                int opp_idx = find_client_index(clients[client_index].current_opponent);
                
                // Kiểm tra xem cả 2 đã hoàn thành chưa
                int my_finished = (clients[client_index].current_question_index >= gs->total_questions);
                // Kiểm tra đối thủ: dựa vào current_question_index HOẶC game đã inactive
                int opp_finished = 0;
                if (opp_idx != -1) {
                    opp_finished = (clients[opp_idx].current_question_index >= gs->total_questions);
                } else if (gs->is_active == 0) {
                    // Game đã kết thúc, nghĩa là đối thủ cũng đã xong
                    opp_finished = 1;
                }
                
                cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_STATUS_UPDATE);
                cJSON_AddNumberToObject(res, "your_total_score", is_player1 ? gs->score1 : gs->score2);
                cJSON_AddNumberToObject(res, "opponent_score", is_player1 ? gs->score2 : gs->score1);
                
                printf("[Server] %s checking: my_finished=%d, opp_finished=%d, game_active=%d\n", 
                    clients[client_index].username, my_finished, opp_finished, gs->is_active);
                
                if (my_finished && opp_finished) {
                    // Cả 2 đều xong
                    printf("[Server] %s checking status: Both finished, sending FINISHED\n", clients[client_index].username);
                    
                    cJSON_AddStringToObject(res, "game_status", "FINISHED");
                    cJSON_AddBoolToObject(res, "you_win", 
                        is_player1 ? (gs->score1 > gs->score2) : (gs->score2 > gs->score1));
                    
                    // Reset trạng thái người chơi này (mỗi người tự reset khi nhận được kết quả)
                    clients[client_index].is_busy = 0;
                    clients[client_index].game_session_id = -1;
                    clients[client_index].current_question_index = 0;
                    memset(clients[client_index].current_opponent, 0, sizeof(clients[client_index].current_opponent));
                } else {
                    // Vẫn đang chờ
                    cJSON_AddStringToObject(res, "game_status", "WAITING_OPPONENT");
                }
            } else {
                cJSON_AddStringToObject(res, "type", "ERROR");
                cJSON_AddStringToObject(res, "message", "Invalid game session");
            }
            
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        
        // --- THOÁT GAME ---
        else if (strcmp(type->valuestring, MSG_TYPE_QUIT_GAME) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            
            int gid = clients[client_index].game_session_id;
            
            // Nếu đang trong game, hủy game session và reset đối thủ
            if (gid >= 0 && gid < MAX_GAME_SESSIONS && game_sessions[gid].is_active) {
                printf("[Server] %s quit game mid-match, ending session %d\n", clients[client_index].username, gid);
                game_sessions[gid].is_active = 0;
                
                // Tìm và reset đối thủ
                int opp_idx = find_client_index(clients[client_index].current_opponent);
                if (opp_idx != -1) {
                    clients[opp_idx].is_busy = 0;
                    clients[opp_idx].game_session_id = -1;
                    clients[opp_idx].current_question_index = 0;
                    memset(clients[opp_idx].current_opponent, 0, sizeof(clients[opp_idx].current_opponent));
                    printf("[Server] Reset opponent %s as well\n", clients[opp_idx].username);
                }
            }
            
            // Reset trạng thái client
            clients[client_index].is_busy = 0;
            clients[client_index].current_question_index = 0;
            clients[client_index].game_session_id = -1;
            memset(clients[client_index].current_opponent, 0, sizeof(clients[client_index].current_opponent));
            
            // Tăng lobby_version vì người chơi quit về FREE
            EnterCriticalSection(&cs_lobby);
            lobby_version++;
            printf("[Server] Player quit game, lobby_version=%d\n", lobby_version);
            LeaveCriticalSection(&cs_lobby);
            
            cJSON_AddStringToObject(res, "type", "QUIT_GAME_SUCCESS");
            
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        
        // --- CÁC CHỨC NĂNG KHÁC ---
        else if (strcmp(type->valuestring, MSG_TYPE_GET_LOBBY_LIST) == 0) {
            // Kiểm tra xem client có yêu cầu include_offline không
            cJSON *include_offline_json = cJSON_GetObjectItem(req, "include_offline");
            int include_offline = (include_offline_json && cJSON_IsTrue(include_offline_json)) ? 1 : 0;
            
            printf("[Server] GET_LOBBY_LIST from %s (include_offline=%d)\n", 
                   clients[client_index].username, include_offline);
            
            cJSON_AddStringToObject(res, "type", MSG_TYPE_LOBBY_LIST);
            cJSON *arr = cJSON_CreateArray();
            
            if (include_offline) {
                // TRẢ VỀ TẤT CẢ NGƯỜI CHƠI ĐÃ ĐĂNG KÝ với status
                char *file_content = read_file(ACCOUNT_FILE);
                if (file_content) {
                    cJSON *accounts = cJSON_Parse(file_content);
                    if (accounts && cJSON_IsArray(accounts)) {
                        cJSON *acc = NULL;
                        cJSON_ArrayForEach(acc, accounts) {
                            cJSON *username_json = cJSON_GetObjectItem(acc, "username");
                            if (username_json) {
                                char *username = username_json->valuestring;
                                
                                // Bỏ qua chính mình
                                if (strcmp(username, clients[client_index].username) == 0) {
                                    continue;
                                }
                                
                                // Tìm xem user này có đang online không
                                EnterCriticalSection(&cs_clients);
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
                                LeaveCriticalSection(&cs_clients);
                                
                                // Tạo object với thông tin đầy đủ
                                cJSON *player_obj = cJSON_CreateObject();
                                cJSON_AddStringToObject(player_obj, "name", username);
                                
                                // Xác định status
                                if (!is_online) {
                                    cJSON_AddStringToObject(player_obj, "status", "OFFLINE");
                                } else if (is_in_game) {
                                    cJSON_AddStringToObject(player_obj, "status", "IN_GAME");
                                } else {
                                    cJSON_AddStringToObject(player_obj, "status", "FREE");
                                }
                                
                                cJSON_AddItemToArray(arr, player_obj);
                            }
                        }
                    }
                    if (accounts) cJSON_Delete(accounts);
                    free(file_content);
                }
                
                printf("[Server] Sent ALL %d players (with status) to %s\n", 
                       cJSON_GetArraySize(arr), clients[client_index].username);
            } else {
                // CHỈ TRẢ VỀ NGƯỜI CHƠI ONLINE VÀ RẢNH (logic cũ)
                EnterCriticalSection(&cs_clients);
                for(int i=0; i<MAX_CLIENTS; i++) {
                    if (clients[i].socket!=0 && clients[i].is_logged_in && i!=client_index && clients[i].is_busy==0) {
                        cJSON_AddItemToArray(arr, cJSON_CreateString(clients[i].username));
                    }
                }
                LeaveCriticalSection(&cs_clients);
                
                printf("[Server] Sent %d online/free players to %s\n", 
                       cJSON_GetArraySize(arr), clients[client_index].username);
            }
            
            cJSON_AddItemToObject(res, "players", arr);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_GET_HISTORY) == 0) {
            // Lấy lịch sử đấu của người chơi hiện tại
            cJSON_AddStringToObject(res, "type", MSG_TYPE_HISTORY_DATA);
            cJSON *history_arr = cJSON_CreateArray();
            
            EnterCriticalSection(&cs_history);
            for (int i = 0; i < history_count; i++) {
                // Chỉ lấy các trận có liên quan đến người chơi này
                if (strcmp(game_history[i].player1, clients[client_index].username) == 0 ||
                    strcmp(game_history[i].player2, clients[client_index].username) == 0) {
                    
                    cJSON *item = cJSON_CreateObject();
                    cJSON_AddNumberToObject(item, "game_key", (double)game_history[i].game_key);
                    cJSON_AddStringToObject(item, "player1", game_history[i].player1);
                    cJSON_AddStringToObject(item, "player2", game_history[i].player2);
                    cJSON_AddNumberToObject(item, "score1", game_history[i].score1);
                    cJSON_AddNumberToObject(item, "score2", game_history[i].score2);
                    cJSON_AddNumberToObject(item, "total_questions", game_history[i].total_questions);
                    cJSON_AddNumberToObject(item, "timestamp", (double)game_history[i].finished_time);
                    
                    // Xác định win/lose/draw cho người chơi này
                    int is_p1 = (strcmp(game_history[i].player1, clients[client_index].username) == 0);
                    int my_score = is_p1 ? game_history[i].score1 : game_history[i].score2;
                    int opp_score = is_p1 ? game_history[i].score2 : game_history[i].score1;
                    char *result = (my_score > opp_score) ? "WIN" : (my_score < opp_score) ? "LOSE" : "DRAW";
                    cJSON_AddStringToObject(item, "result", result);
                    
                    cJSON_AddItemToArray(history_arr, item);
                }
            }
            LeaveCriticalSection(&cs_history);
            
            cJSON_AddItemToObject(res, "history", history_arr);
            printf("[Server] Sent %d history records to %s\n", cJSON_GetArraySize(history_arr), clients[client_index].username);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_LOGOUT) == 0) {
            // Xử lý đăng xuất
            EnterCriticalSection(&cs_clients);
            char logout_user[50];
            strcpy(logout_user, clients[client_index].username);
            
            // Kiểm tra xem có đang trong game không
            if (clients[client_index].is_busy || clients[client_index].game_session_id >= 0) {
                LeaveCriticalSection(&cs_clients);
                cJSON_AddStringToObject(res, "type", "LOGOUT_FAIL");
                cJSON_AddStringToObject(res, "message", "Khong the dang xuat khi dang choi");
                printf("[Server] %s tried to logout while in game\n", logout_user);
            } else {
                // Đánh dấu logout
                clients[client_index].is_logged_in = 0;
                LeaveCriticalSection(&cs_clients);
                
                // Tăng lobby version
                EnterCriticalSection(&cs_lobby);
                lobby_version++;
                printf("[Server] User %s logged out, lobby_version=%d\n", logout_user, lobby_version);
                LeaveCriticalSection(&cs_lobby);
                
                cJSON_AddStringToObject(res, "type", MSG_TYPE_LOGOUT_SUCCESS);
                printf("[Server] %s logged out successfully\n", logout_user);
            }
        }
        else if (strcmp(type->valuestring, MSG_TYPE_GET_LEADERBOARD) == 0) {
            cJSON_AddStringToObject(res, "type", MSG_TYPE_LEADERBOARD_DATA);
            cJSON_AddItemToObject(res, "players", get_leaderboard_json());
        }

        char *res_str = cJSON_PrintUnformatted(res);
        send(client_socket, res_str, strlen(res_str), 0);
        free(res_str);
        cJSON_Delete(res);
        cJSON_Delete(req);
    }

    // 3. Ngắt kết nối
    EnterCriticalSection(&cs_clients);
    char disconnected_user[50];
    strcpy(disconnected_user, clients[client_index].username);
    int was_logged_in = clients[client_index].is_logged_in;
    clients[client_index].socket = 0;
    LeaveCriticalSection(&cs_clients);
    
    // Tăng lobby version nếu user đã login
    if (was_logged_in && strlen(disconnected_user) > 0) {
        EnterCriticalSection(&cs_lobby);
        lobby_version++;
        printf("[Server] User %s disconnected, lobby_version=%d\n", disconnected_user, lobby_version);
        LeaveCriticalSection(&cs_lobby);
    }
    
    closesocket(client_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    InitializeCriticalSection(&cs_clients);
    InitializeCriticalSection(&cs_games);
    InitializeCriticalSection(&cs_history);
    InitializeCriticalSection(&cs_lobby);
    
    // Khởi tạo game sessions và history
    for (int i = 0; i < MAX_GAME_SESSIONS; i++) {
        game_sessions[i].is_active = 0;
    }
    
    // LOAD LỊCH SỬ TỪ FILE KHI KHỞI ĐỘNG
    load_history_from_file();
    
    srand(time(NULL)); // Seed cho random
    WSAStartup(MAKEWORD(2, 2), &wsa);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("[Server] Dang chay tai port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket != INVALID_SOCKET) {
            CreateThread(NULL, 0, handle_client, (LPVOID)new_socket, 0, NULL);
        }
    }
    return 0;
}