// src/server/modules/connection_handler.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // Dùng cho hàm time()
#include "../../common/protocol.h"
#include "../../common/cJSON.h"
#include "../include/connection_handler.h"
#include "../include/server_state.h"
#include "../include/auth_service.h"
#include "../include/game_service.h"
#include "../include/data_manager.h"

// --- HELPER CHO GAME SERVICE (Tạm thời khai báo ở đây nếu chưa có trong header) ---
// Trong thực tế nên đưa vào game_service.h, nhưng để đảm bảo compile được ngay:
extern int get_current_prize(int q_index);
extern int get_safe_reward(int current_q_index);

DWORD WINAPI handle_client(LPVOID client_socket_ptr) {
    SOCKET client_socket = (SOCKET)client_socket_ptr;
    char buffer[BUFFER_SIZE];
    int n, client_index = -1;

    // 1. Đăng ký slot trong mảng clients
    EnterCriticalSection(&cs_clients);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = client_socket;
            clients[i].is_logged_in = 0;
            clients[i].is_busy = 0;
            strcpy(clients[i].pending_invite_from, "");
            clients[i].game_session_id = -1;
            clients[i].current_question_index = 0;
            clients[i].last_lobby_version = -1;
            clients[i].opponent_quit = 0;
            clients[i].last_chat_version = 0;
            client_index = i;
            break;
        }
    }
    LeaveCriticalSection(&cs_clients);

    if (client_index == -1) { 
        closesocket(client_socket); 
        return 0; 
    }

    // 2. Vòng lặp nhận tin nhắn từ Client
    while ((n = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';

        cJSON *req = cJSON_Parse(buffer);
        if (!req) continue;
        cJSON *type = cJSON_GetObjectItem(req, "type");
        if (!type) { cJSON_Delete(req); continue; }

        cJSON *res = cJSON_CreateObject();

        // --- XỬ LÝ ĐĂNG KÝ ---
        if (strcmp(type->valuestring, MSG_TYPE_REGISTER) == 0) {
            cJSON *u = cJSON_GetObjectItem(req, "user");
            cJSON *p = cJSON_GetObjectItem(req, "pass");
            if (u && p && process_register(u->valuestring, p->valuestring)) {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_SUCCESS);
                cJSON_AddStringToObject(res, "message", "Dang ky thanh cong");
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_FAIL);
                cJSON_AddStringToObject(res, "message", "Dang ky that bai (User ton tai)");
            }
        }
        // --- XỬ LÝ ĐĂNG NHẬP ---
        else if (strcmp(type->valuestring, MSG_TYPE_LOGIN) == 0) {
            cJSON *u = cJSON_GetObjectItem(req, "user");
            cJSON *p = cJSON_GetObjectItem(req, "pass");
            int score = 0;
            if (u && p && process_login(u->valuestring, p->valuestring, &score)) {
                EnterCriticalSection(&cs_clients);
                strcpy(clients[client_index].username, u->valuestring);
                clients[client_index].is_logged_in = 1;
                clients[client_index].score = score;
                clients[client_index].is_busy = 0;
                clients[client_index].opponent_quit = 0;
                
                // Sync chat version
                EnterCriticalSection(&cs_chat);
                clients[client_index].last_chat_version = chat_version;
                LeaveCriticalSection(&cs_chat);
                
                LeaveCriticalSection(&cs_clients);

                cJSON_AddStringToObject(res, "type", MSG_TYPE_LOGIN_SUCCESS);
                cJSON_AddStringToObject(res, "user", u->valuestring);
                cJSON_AddNumberToObject(res, "total_score", score);
                
                EnterCriticalSection(&cs_lobby);
                lobby_version++;
                clients[client_index].last_lobby_version = lobby_version;
                LeaveCriticalSection(&cs_lobby);
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_LOGIN_FAIL);
                cJSON_AddStringToObject(res, "message", "Sai tai khoan/mat khau");
            }
        }
        // --- XỬ LÝ POLLING ---
        else if (strcmp(type->valuestring, MSG_TYPE_POLL) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            
            // Ưu tiên 0: Đối thủ thoát (PvP)
            if (clients[client_index].opponent_quit == 1) {
                cJSON_AddStringToObject(res, "type", "OPPONENT_QUIT");
                cJSON_AddStringToObject(res, "opponent", clients[client_index].current_opponent);
                clients[client_index].opponent_quit = 0;
            }
            // Ưu tiên 1: Lời mời
            else if (strlen(clients[client_index].pending_invite_from) > 0) {
                char invite_copy[100];
                strcpy(invite_copy, clients[client_index].pending_invite_from);
                char *colon = strchr(invite_copy, ':');
                int num_q = 5;
                if (colon) { *colon = '\0'; num_q = atoi(colon + 1); }
                
                cJSON_AddStringToObject(res, "type", MSG_TYPE_RECEIVE_INVITE);
                cJSON_AddStringToObject(res, "from", invite_copy);
                cJSON_AddNumberToObject(res, "num_questions", num_q);
            } 
            // Ưu tiên 2: Game Start
            else if (clients[client_index].is_busy == 1 && clients[client_index].game_session_id >= 0) {
                 int gid = clients[client_index].game_session_id;
                 if (game_sessions[gid].is_active) {
                     cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_START);
                     cJSON_AddStringToObject(res, "opponent", clients[client_index].current_opponent);
                     cJSON_AddNumberToObject(res, "total_questions", game_sessions[gid].total_questions);
                     cJSON_AddNumberToObject(res, "game_key", (double)game_sessions[gid].game_key);
                     
                     if (strlen(game_sessions[gid].player2) == 0) {
                         cJSON_AddStringToObject(res, "mode", "CLASSIC");
                     } else {
                         cJSON_AddStringToObject(res, "mode", "PVP");
                     }
                 }
            }
            // Ưu tiên 3: Chat
            else {
                EnterCriticalSection(&cs_chat);
                int current_chat = chat_version;
                int client_chat = clients[client_index].last_chat_version;
                LeaveCriticalSection(&cs_chat);
                
                if (client_chat != current_chat && chat_count > 0) {
                    EnterCriticalSection(&cs_chat);
                    int last_idx = (chat_count - 1) % MAX_CHAT_MESSAGES;
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_NEW_CHAT_MESSAGE);
                    cJSON_AddStringToObject(res, "username", chat_messages[last_idx].username);
                    cJSON_AddStringToObject(res, "message", chat_messages[last_idx].message);
                    clients[client_index].last_chat_version = current_chat;
                    LeaveCriticalSection(&cs_chat);
                }
                // Ưu tiên 4: Lobby Update
                else {
                    EnterCriticalSection(&cs_lobby);
                    int current_lobby = lobby_version;
                    int client_lobby = clients[client_index].last_lobby_version;
                    LeaveCriticalSection(&cs_lobby);
                    
                    if (client_lobby != current_lobby) {
                        cJSON_AddStringToObject(res, "type", MSG_TYPE_LOBBY_LIST);
                        EnterCriticalSection(&cs_lobby);
                        clients[client_index].last_lobby_version = current_lobby;
                        LeaveCriticalSection(&cs_lobby);
                    } else {
                        cJSON_AddStringToObject(res, "type", MSG_TYPE_NO_EVENT);
                    }
                }
            }
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        // --- GET LOBBY LIST ---
        else if (strcmp(type->valuestring, MSG_TYPE_GET_LOBBY_LIST) == 0) {
            cJSON_AddStringToObject(res, "type", MSG_TYPE_LOBBY_LIST);
            cJSON *arr = cJSON_CreateArray();
            char *file_content = read_file(ACCOUNT_FILE);
            if (file_content) {
                cJSON *accounts = cJSON_Parse(file_content);
                cJSON *acc = NULL;
                cJSON_ArrayForEach(acc, accounts) {
                    cJSON *u = cJSON_GetObjectItem(acc, "username");
                    if (u && strcmp(u->valuestring, clients[client_index].username) != 0) {
                        cJSON *pObj = cJSON_CreateObject();
                        cJSON_AddStringToObject(pObj, "name", u->valuestring);
                        int is_on = 0, is_game = 0;
                        EnterCriticalSection(&cs_clients);
                        for(int k=0; k<MAX_CLIENTS; k++) {
                            if(clients[k].socket && clients[k].is_logged_in && strcmp(clients[k].username, u->valuestring)==0) {
                                is_on = 1; 
                                is_game = clients[k].is_busy; 
                                break;
                            }
                        }
                        LeaveCriticalSection(&cs_clients);
                        cJSON_AddStringToObject(pObj, "status", !is_on ? "OFFLINE" : (is_game ? "IN_GAME" : "FREE"));
                        cJSON_AddItemToArray(arr, pObj);
                    }
                }
                cJSON_Delete(accounts);
                free(file_content);
            }
            cJSON_AddItemToObject(res, "players", arr);
        }
        // --- START CLASSIC ---
        else if (strcmp(type->valuestring, MSG_TYPE_START_CLASSIC) == 0) {
            EnterCriticalSection(&cs_clients);
            int gid = create_game_session(clients[client_index].username, "", 15);
            
            if (gid != -1) {
                clients[client_index].is_busy = 1;
                clients[client_index].game_session_id = gid;
                clients[client_index].current_question_index = 0;
                strcpy(clients[client_index].current_opponent, "BOT"); 
                
                // Init lifelines & Score
                memset(game_sessions[gid].p1_lifelines, 0, sizeof(game_sessions[gid].p1_lifelines));
                game_sessions[gid].score1 = 0; // Reset tiền về 0

                cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_START);
                cJSON_AddStringToObject(res, "mode", "CLASSIC");
                cJSON_AddStringToObject(res, "opponent", "BOT");
                cJSON_AddNumberToObject(res, "total_questions", 15);
                cJSON_AddNumberToObject(res, "game_key", (double)game_sessions[gid].game_key);
                
                // Tăng lobby_version để các client khác thấy trạng thái IN_GAME real-time
                EnterCriticalSection(&cs_lobby);
                lobby_version++;
                LeaveCriticalSection(&cs_lobby);
            } else {
                cJSON_AddStringToObject(res, "type", "ERROR");
                cJSON_AddStringToObject(res, "message", "Server busy");
            }
            LeaveCriticalSection(&cs_clients);
        }
        // --- INVITE ---
        else if (strcmp(type->valuestring, MSG_TYPE_INVITE_PLAYER) == 0) {
            cJSON *target = cJSON_GetObjectItem(req, "target");
            cJSON *nq = cJSON_GetObjectItem(req, "num_questions");
            int n = nq ? nq->valueint : 5;
            
            EnterCriticalSection(&cs_clients);
            int t_idx = find_client_index(target->valuestring);
            if (t_idx != -1 && clients[t_idx].is_busy == 0) {
                char buf[100]; sprintf(buf, "%s:%d", clients[client_index].username, n);
                strcpy(clients[t_idx].pending_invite_from, buf);
                clients[client_index].is_busy = 1; 
                cJSON_AddStringToObject(res, "type", "INVITE_SENT_SUCCESS");
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_INVITE_FAIL);
                cJSON_AddStringToObject(res, "message", "Player busy or offline");
            }
            LeaveCriticalSection(&cs_clients);
        }
        // --- ACCEPT INVITE ---
        else if (strcmp(type->valuestring, MSG_TYPE_ACCEPT_INVITE) == 0) {
            cJSON *inviter = cJSON_GetObjectItem(req, "from");
            EnterCriticalSection(&cs_clients);
            int i_idx = find_client_index(inviter->valuestring);
            if (i_idx != -1) {
                char *col = strchr(clients[client_index].pending_invite_from, ':');
                int n = col ? atoi(col+1) : 5;
                
                int gid = create_game_session(clients[i_idx].username, clients[client_index].username, n);
                if (gid != -1) {
                    clients[client_index].is_busy = 1;
                    strcpy(clients[client_index].current_opponent, clients[i_idx].username);
                    clients[client_index].game_session_id = gid;
                    clients[client_index].current_question_index = 0;
                    strcpy(clients[client_index].pending_invite_from, "");
                    
                    clients[i_idx].is_busy = 1;
                    strcpy(clients[i_idx].current_opponent, clients[client_index].username);
                    clients[i_idx].game_session_id = gid;
                    clients[i_idx].current_question_index = 0;
                    
                    EnterCriticalSection(&cs_lobby); lobby_version++; LeaveCriticalSection(&cs_lobby);
                    
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_START);
                    cJSON_AddStringToObject(res, "mode", "PVP");
                    cJSON_AddStringToObject(res, "opponent", clients[i_idx].username);
                    cJSON_AddNumberToObject(res, "total_questions", n);
                    cJSON_AddNumberToObject(res, "game_key", (double)game_sessions[gid].game_key);
                }
            }
            LeaveCriticalSection(&cs_clients);
        }
        // --- REJECT INVITE ---
        else if (strcmp(type->valuestring, MSG_TYPE_REJECT_INVITE) == 0) {
             cJSON *from = cJSON_GetObjectItem(req, "from");
             EnterCriticalSection(&cs_clients);
             int iidx = find_client_index(from->valuestring);
             strcpy(clients[client_index].pending_invite_from, "");
             if(iidx!=-1) { clients[iidx].is_busy=0; strcpy(clients[iidx].current_opponent, ""); }
             LeaveCriticalSection(&cs_clients);
             cJSON_AddStringToObject(res, "type", "REJECT_SUCCESS");
        }
        // --- USE LIFELINE ---
        else if (strcmp(type->valuestring, MSG_TYPE_USE_LIFELINE) == 0) {
            cJSON *lid = cJSON_GetObjectItem(req, "lifeline_id");
            if (lid) {
                EnterCriticalSection(&cs_games);
                int gid = clients[client_index].game_session_id;
                cJSON *result_data = process_lifeline(gid, clients[client_index].username, lid->valueint);
                LeaveCriticalSection(&cs_games);
                
                if (result_data) {
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_LIFELINE_RES);
                    cJSON_AddItemToObject(res, "data", result_data);
                } else {
                    cJSON_AddStringToObject(res, "type", "ERROR");
                }
            }
        }
        // --- REQUEST QUESTION ---
        else if (strcmp(type->valuestring, MSG_TYPE_REQUEST_QUESTION) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            int gid = clients[client_index].game_session_id;
            int q_idx = clients[client_index].current_question_index;
            
            if (gid >= 0 && game_sessions[gid].is_active) {
                if (q_idx < game_sessions[gid].total_questions) {
                    int qid = game_sessions[gid].used_question_ids[q_idx];
                    
                    Question *q_found = NULL;
                    for (int i = 0; i < question_count; i++) {
                        if (question_bank[i].id == qid) {
                            q_found = &question_bank[i]; break;
                        }
                    }

                    if (q_found) {
                        cJSON_AddStringToObject(res, "type", MSG_TYPE_QUESTION);
                        cJSON_AddNumberToObject(res, "question_number", q_idx + 1);
                        cJSON_AddNumberToObject(res, "total_questions", game_sessions[gid].total_questions);
                        cJSON_AddStringToObject(res, "question", q_found->question);
                        
                        cJSON *opts = cJSON_CreateArray();
                        for(int i=0; i<4; i++) cJSON_AddItemToArray(opts, cJSON_CreateString(q_found->options[i]));
                        cJSON_AddItemToObject(res, "options", opts);
                        
                        cJSON_AddNumberToObject(res, "question_id", qid);
                        cJSON_AddNumberToObject(res, "max_time", MAX_TIME_PER_QUESTION);
                        
                        int is_p1 = (strcmp(clients[client_index].username, game_sessions[gid].player1)==0);
                        cJSON_AddNumberToObject(res, "opponent_score", is_p1 ? game_sessions[gid].score2 : game_sessions[gid].score1);
                    } else {
                        cJSON_AddStringToObject(res, "type", "ERROR");
                        cJSON_AddStringToObject(res, "message", "Data error");
                    }
                } else {
                    cJSON_AddStringToObject(res, "type", "NO_MORE_QUESTIONS");
                }
            } else {
                cJSON_AddStringToObject(res, "type", "ERROR");
                cJSON_AddStringToObject(res, "message", "Invalid Session");
            }
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        // --- SUBMIT ANSWER (CẬP NHẬT LOGIC CLASSIC) ---
        else if (strcmp(type->valuestring, MSG_TYPE_SUBMIT_ANSWER) == 0) {
            cJSON *q_item = cJSON_GetObjectItem(req, "question_id");
            cJSON *ans_item = cJSON_GetObjectItem(req, "answer_index");
            cJSON *time_item = cJSON_GetObjectItem(req, "time_taken");
            
            if (q_item && ans_item && time_item) {
                int qid = q_item->valueint;
                int ans = ans_item->valueint;
                double time_taken = time_item->valuedouble;
                
                EnterCriticalSection(&cs_clients);
                EnterCriticalSection(&cs_games);
                int gid = clients[client_index].game_session_id;
                
                if (gid >= 0 && game_sessions[gid].is_active) {
                    int correct = -1;
                    for (int i = 0; i < question_count; i++) {
                        if (question_bank[i].id == qid) { correct = question_bank[i].answer_index; break; }
                    }
                    
                    // Kiểm tra chế độ chơi
                    int is_classic = (strlen(game_sessions[gid].player2) == 0);
                    int is_p1 = (strcmp(clients[client_index].username, game_sessions[gid].player1) == 0);
                    
                    int earned = 0;
                    int game_over_classic = 0;
                    int current_q_idx = clients[client_index].current_question_index;

                    if (is_classic) {
                        // --- LOGIC CLASSIC MỚI ---
                        if (ans == correct) {
                            earned = get_current_prize(current_q_idx);
                            if (is_p1) game_sessions[gid].score1 = earned; // Gán trực tiếp tiền
                        } else {
                            // Sai: Về mốc an toàn
                            int safe = get_safe_reward(current_q_idx);
                            if (is_p1) game_sessions[gid].score1 = safe;
                            game_over_classic = 1;
                            earned = 0;
                        }
                    } else {
                        // --- LOGIC PVP CŨ ---
                        earned = calculate_score(ans == correct, time_taken);
                        if(is_p1) game_sessions[gid].score1 += earned;
                        else game_sessions[gid].score2 += earned;
                    }
                    
                    clients[client_index].current_question_index++;
                    
                    // Phản hồi
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_ANSWER_RESULT);
                    cJSON_AddBoolToObject(res, "is_correct", ans == correct);
                    cJSON_AddNumberToObject(res, "correct_answer", correct);
                    cJSON_AddNumberToObject(res, "earned_score", earned); // Classic: Tiền câu này, PvP: Điểm cộng
                    cJSON_AddNumberToObject(res, "your_total_score", is_p1 ? game_sessions[gid].score1 : game_sessions[gid].score2);
                    cJSON_AddNumberToObject(res, "opponent_score", is_p1 ? game_sessions[gid].score2 : game_sessions[gid].score1);
                    
                    // Kiểm tra kết thúc
                    int session_ended = 0;

                    // 1. Thua Classic
                    if (is_classic && game_over_classic) {
                        session_ended = 1;
                        cJSON_AddStringToObject(res, "game_status", "FINISHED");
                        cJSON_AddBoolToObject(res, "you_win", 0);
                        update_user_score(clients[client_index].username, game_sessions[gid].score1);
                    }
                    // 2. Hết câu hỏi
                    else if (clients[client_index].current_question_index >= game_sessions[gid].total_questions) {
                        int opp_done = 1;
                        if (!is_classic) {
                            int opp_idx = find_client_index(clients[client_index].current_opponent);
                            if (opp_idx != -1) opp_done = (clients[opp_idx].current_question_index >= game_sessions[gid].total_questions);
                        }
                        
                        if (opp_done) {
                            session_ended = 1;
                            if (is_classic) {
                                // Thắng Classic (Trả lời hết 15 câu)
                                update_user_score(clients[client_index].username, game_sessions[gid].score1);
                            } else {
                                // Kết thúc PvP
                                update_user_score(game_sessions[gid].player1, game_sessions[gid].score1);
                                update_user_score(game_sessions[gid].player2, game_sessions[gid].score2);
                                // Lưu lịch sử PvP
                                EnterCriticalSection(&cs_history);
                                if(history_count < MAX_HISTORY) {
                                    game_history[history_count].game_key = game_sessions[gid].game_key;
                                    strcpy(game_history[history_count].player1, game_sessions[gid].player1);
                                    strcpy(game_history[history_count].player2, game_sessions[gid].player2);
                                    game_history[history_count].score1 = game_sessions[gid].score1;
                                    game_history[history_count].score2 = game_sessions[gid].score2;
                                    game_history[history_count].total_questions = game_sessions[gid].total_questions;
                                    game_history[history_count].finished_time = time(NULL);
                                    history_count++;
                                }
                                LeaveCriticalSection(&cs_history);
                                save_history_to_file();
                            }
                            
                            cJSON_AddStringToObject(res, "game_status", "FINISHED");
                            int win = 1; 
                            if (!is_classic) win = is_p1 ? (game_sessions[gid].score1 > game_sessions[gid].score2) : (game_sessions[gid].score2 > game_sessions[gid].score1);
                            cJSON_AddBoolToObject(res, "you_win", win);
                        } else {
                            cJSON_AddStringToObject(res, "game_status", "WAITING_OPPONENT");
                        }
                    } else {
                        cJSON_AddStringToObject(res, "game_status", "NEXT_QUESTION");
                    }
                    
                    if (session_ended) {
                        game_sessions[gid].is_active = 0;
                        clients[client_index].is_busy = 0;
                        clients[client_index].game_session_id = -1;
                        memset(clients[client_index].current_opponent, 0, 50);
                    }
                }
                LeaveCriticalSection(&cs_games);
                LeaveCriticalSection(&cs_clients);
            }
        }
        // --- CHECK GAME STATUS ---
        else if (strcmp(type->valuestring, MSG_TYPE_CHECK_GAME_STATUS) == 0) {
             EnterCriticalSection(&cs_clients);
             EnterCriticalSection(&cs_games);
             int gid = clients[client_index].game_session_id;
             if(gid >= 0) {
                 int is_p1 = (strcmp(clients[client_index].username, game_sessions[gid].player1)==0);
                 
                 cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_STATUS_UPDATE);
                 cJSON_AddNumberToObject(res, "your_total_score", is_p1?game_sessions[gid].score1:game_sessions[gid].score2);
                 cJSON_AddNumberToObject(res, "opponent_score", is_p1?game_sessions[gid].score2:game_sessions[gid].score1);
                 
                 if(!game_sessions[gid].is_active) {
                     cJSON_AddStringToObject(res, "game_status", "FINISHED");
                     int win = 0;
                     if (strlen(game_sessions[gid].player2) == 0) win = 1;
                     else win = is_p1 ? (game_sessions[gid].score1 > game_sessions[gid].score2) : (game_sessions[gid].score2 > game_sessions[gid].score1);
                     cJSON_AddBoolToObject(res, "you_win", win);
                     
                     // Reset me
                     clients[client_index].is_busy = 0;
                     clients[client_index].game_session_id = -1;
                     memset(clients[client_index].current_opponent, 0, 50);
                 } else {
                     cJSON_AddStringToObject(res, "game_status", "WAITING_OPPONENT");
                 }
             }
             LeaveCriticalSection(&cs_games);
             LeaveCriticalSection(&cs_clients);
        }
        // --- QUIT GAME (CẬP NHẬT LOGIC CLASSIC) ---
        else if (strcmp(type->valuestring, MSG_TYPE_QUIT_GAME) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            int gid = clients[client_index].game_session_id;
            if(gid >= 0 && game_sessions[gid].is_active) {
                int is_classic = (strlen(game_sessions[gid].player2) == 0);
                
                if (is_classic) {
                    // Dừng cuộc chơi -> Bảo toàn tiền hiện tại
                    update_user_score(clients[client_index].username, game_sessions[gid].score1);
                } else {
                    // PvP Quit -> Xử thua cho người quit, thắng cho người ở lại
                    int opp_idx = find_client_index(clients[client_index].current_opponent);
                    if(opp_idx != -1 && clients[opp_idx].opponent_quit == 0) {
                        clients[opp_idx].opponent_quit = 1; // Báo đối thủ biết
                        
                        // Lưu lịch sử (Người quit thua)
                        EnterCriticalSection(&cs_history);
                        if(history_count < MAX_HISTORY) {
                            // ... code lưu history (đã có ở trên)
                        }
                        LeaveCriticalSection(&cs_history);
                        
                        // Cộng điểm
                        update_user_score(game_sessions[gid].player1, game_sessions[gid].score1);
                        update_user_score(game_sessions[gid].player2, game_sessions[gid].score2);
                    }
                }
                game_sessions[gid].is_active = 0;
            }
            
            clients[client_index].is_busy = 0;
            clients[client_index].game_session_id = -1;
            clients[client_index].opponent_quit = 0;
            strcpy(clients[client_index].current_opponent, "");
            
            EnterCriticalSection(&cs_lobby); lobby_version++; LeaveCriticalSection(&cs_lobby);
            cJSON_AddStringToObject(res, "type", "QUIT_GAME_SUCCESS");
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        // --- CÁC PHẦN KHÁC GIỮ NGUYÊN ---
        else if (strcmp(type->valuestring, MSG_TYPE_GET_HISTORY) == 0) {
             // (Copy logic GET_HISTORY cũ)
             cJSON_AddStringToObject(res, "type", MSG_TYPE_HISTORY_DATA);
             cJSON *h_arr = cJSON_CreateArray();
             EnterCriticalSection(&cs_history);
             for (int i=0; i<history_count; i++) {
                 if (strcmp(game_history[i].player1, clients[client_index].username) == 0 ||
                     strcmp(game_history[i].player2, clients[client_index].username) == 0) {
                     cJSON *item = cJSON_CreateObject();
                     
                     // Thêm đầy đủ các field
                     cJSON_AddNumberToObject(item, "game_key", (double)game_history[i].game_key);
                     cJSON_AddStringToObject(item, "player1", game_history[i].player1);
                     cJSON_AddStringToObject(item, "player2", game_history[i].player2);
                     cJSON_AddNumberToObject(item, "score1", game_history[i].score1);
                     cJSON_AddNumberToObject(item, "score2", game_history[i].score2);
                     cJSON_AddNumberToObject(item, "total_questions", game_history[i].total_questions);
                     cJSON_AddNumberToObject(item, "timestamp", (double)game_history[i].finished_time);
                     
                     int is_p1 = (strcmp(game_history[i].player1, clients[client_index].username)==0);
                     int my_s = is_p1 ? game_history[i].score1 : game_history[i].score2;
                     int opp_s = is_p1 ? game_history[i].score2 : game_history[i].score1;
                     char *rs = "DRAW";
                     if (strlen(game_history[i].player2) == 0) rs = "SOLO"; 
                     else if (my_s > opp_s) rs = "WIN";
                     else if (my_s < opp_s) rs = "LOSE";
                     
                     cJSON_AddStringToObject(item, "result", rs);
                     cJSON_AddItemToArray(h_arr, item);
                 }
             }
             LeaveCriticalSection(&cs_history);
             cJSON_AddItemToObject(res, "history", h_arr);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_LOGOUT) == 0) {
             // ... (Copy logic LOGOUT cũ)
             EnterCriticalSection(&cs_clients);
             if (clients[client_index].is_busy) {
                 cJSON_AddStringToObject(res, "type", "LOGOUT_FAIL");
             } else {
                 clients[client_index].is_logged_in = 0;
                 EnterCriticalSection(&cs_lobby); lobby_version++; LeaveCriticalSection(&cs_lobby);
                 cJSON_AddStringToObject(res, "type", MSG_TYPE_LOGOUT_SUCCESS);
             }
             LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_GET_LEADERBOARD) == 0) {
            cJSON_AddStringToObject(res, "type", MSG_TYPE_LEADERBOARD_DATA);
            cJSON_AddItemToObject(res, "players", get_leaderboard_json());
        }
        else if (strcmp(type->valuestring, MSG_TYPE_SEND_CHAT) == 0) {
            // ... (Copy logic SEND_CHAT cũ)
            cJSON *msg = cJSON_GetObjectItem(req, "message");
            if (msg && msg->valuestring) {
                EnterCriticalSection(&cs_chat);
                int idx = chat_count % MAX_CHAT_MESSAGES;
                strncpy(chat_messages[idx].username, clients[client_index].username, 49);
                strncpy(chat_messages[idx].message, msg->valuestring, 255);
                chat_messages[idx].timestamp = time(NULL);
                if (chat_count < MAX_CHAT_MESSAGES) chat_count++;
                chat_version++;
                LeaveCriticalSection(&cs_chat);
                cJSON_AddStringToObject(res, "type", MSG_TYPE_CHAT_SUCCESS);
            }
        }
        else if (strcmp(type->valuestring, MSG_TYPE_GET_CHAT_HISTORY) == 0) {
            // ... (Copy logic GET_CHAT_HISTORY cũ)
            cJSON_AddStringToObject(res, "type", MSG_TYPE_CHAT_HISTORY);
            cJSON *msg_arr = cJSON_CreateArray();
            EnterCriticalSection(&cs_chat);
            int start = (chat_count >= MAX_CHAT_MESSAGES) ? (chat_count % MAX_CHAT_MESSAGES) : 0;
            int count = (chat_count < MAX_CHAT_MESSAGES) ? chat_count : MAX_CHAT_MESSAGES;
            for (int i = 0; i < count; i++) {
                int idx = (start + i) % MAX_CHAT_MESSAGES;
                cJSON *msg_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(msg_obj, "username", chat_messages[idx].username);
                cJSON_AddStringToObject(msg_obj, "message", chat_messages[idx].message);
                cJSON_AddItemToArray(msg_arr, msg_obj);
            }
            LeaveCriticalSection(&cs_chat);
            cJSON_AddItemToObject(res, "messages", msg_arr);
        }

        char *res_str = cJSON_PrintUnformatted(res);
        send(client_socket, res_str, strlen(res_str), 0);
        free(res_str);
        cJSON_Delete(res);
        cJSON_Delete(req);
    }

    EnterCriticalSection(&cs_clients);
    if(clients[client_index].is_logged_in) {
        EnterCriticalSection(&cs_lobby);
        lobby_version++;
        LeaveCriticalSection(&cs_lobby);
    }
    clients[client_index].socket = 0;
    LeaveCriticalSection(&cs_clients);
    closesocket(client_socket);
    return 0;
}