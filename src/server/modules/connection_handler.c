#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../common/protocol.h"
#include "../../common/cJSON.h"
#include "../include/connection_handler.h"
#include "../include/server_state.h"
#include "../include/auth_service.h"
#include "../include/game_service.h"
#include "../include/data_manager.h"

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
            clients[i].last_lobby_version = -1;
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

        if (strcmp(type->valuestring, MSG_TYPE_REGISTER) == 0) {
            cJSON *u = cJSON_GetObjectItem(req, "user");
            cJSON *p = cJSON_GetObjectItem(req, "pass");
            if (u && p && process_register(u->valuestring, p->valuestring)) {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_SUCCESS);
                cJSON_AddStringToObject(res, "message", "Dang ky thanh cong");
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_REGISTER_FAIL);
                cJSON_AddStringToObject(res, "message", "Dang ky that bai (User ton tai hoac loi)");
            }
        }
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
        else if (strcmp(type->valuestring, MSG_TYPE_POLL) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            
            if (strlen(clients[client_index].pending_invite_from) > 0) {
                char invite_copy[100];
                strcpy(invite_copy, clients[client_index].pending_invite_from);
                char *colon = strchr(invite_copy, ':');
                int num_q = 5;
                if (colon) { *colon = '\0'; num_q = atoi(colon + 1); }
                
                cJSON_AddStringToObject(res, "type", MSG_TYPE_RECEIVE_INVITE);
                cJSON_AddStringToObject(res, "from", invite_copy);
                cJSON_AddNumberToObject(res, "num_questions", num_q);
            } 
            else if (clients[client_index].is_busy == 1 && strlen(clients[client_index].current_opponent) > 0 && clients[client_index].game_session_id >= 0) {
                int gid = clients[client_index].game_session_id;
                cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_START);
                cJSON_AddStringToObject(res, "opponent", clients[client_index].current_opponent);
                cJSON_AddNumberToObject(res, "total_questions", game_sessions[gid].total_questions);
                cJSON_AddNumberToObject(res, "game_key", (double)game_sessions[gid].game_key);
            }
            else {
                EnterCriticalSection(&cs_lobby);
                int current_lobby = lobby_version;
                int client_lobby = clients[client_index].last_lobby_version;
                LeaveCriticalSection(&cs_lobby);
                
                if (client_lobby != current_lobby) {
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_LOBBY_LIST);
                    cJSON *arr = cJSON_CreateArray();
                    char *file_content = read_file(ACCOUNT_FILE);
                    if (file_content) {
                        cJSON *accounts = cJSON_Parse(file_content);
                        cJSON *acc = NULL; // <--- FIX 2: Khai bao acc
                        cJSON_ArrayForEach(acc, accounts) {
                            cJSON *u = cJSON_GetObjectItem(acc, "username");
                            if(u && strcmp(u->valuestring, clients[client_index].username) != 0) {
                                cJSON *pObj = cJSON_CreateObject();
                                cJSON_AddStringToObject(pObj, "name", u->valuestring);
                                int is_on = 0, is_game = 0;
                                for(int k=0; k<MAX_CLIENTS; k++) {
                                    if(clients[k].socket && clients[k].is_logged_in && strcmp(clients[k].username, u->valuestring)==0) {
                                        is_on=1; is_game=clients[k].is_busy; break;
                                    }
                                }
                                cJSON_AddStringToObject(pObj, "status", !is_on?"OFFLINE":(is_game?"IN_GAME":"FREE"));
                                cJSON_AddItemToArray(arr, pObj);
                            }
                        }
                        cJSON_Delete(accounts); free(file_content);
                    }
                    cJSON_AddItemToObject(res, "players", arr);
                    
                    EnterCriticalSection(&cs_lobby);
                    clients[client_index].last_lobby_version = current_lobby;
                    LeaveCriticalSection(&cs_lobby);
                } else {
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_NO_EVENT);
                }
            }
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_GET_LOBBY_LIST) == 0) {
            cJSON_AddStringToObject(res, "type", MSG_TYPE_LOBBY_LIST);
            cJSON *arr = cJSON_CreateArray();
            char *file_content = read_file(ACCOUNT_FILE);
            if (file_content) {
                cJSON *accounts = cJSON_Parse(file_content);
                cJSON *acc = NULL; // <--- FIX 2: Khai bao acc
                cJSON_ArrayForEach(acc, accounts) {
                    cJSON *u = cJSON_GetObjectItem(acc, "username");
                    if (u && strcmp(u->valuestring, clients[client_index].username) != 0) {
                        cJSON *pObj = cJSON_CreateObject();
                        cJSON_AddStringToObject(pObj, "name", u->valuestring);
                        int is_on = 0, is_game = 0;
                        EnterCriticalSection(&cs_clients);
                        for(int k=0; k<MAX_CLIENTS; k++) {
                            if(clients[k].socket && clients[k].is_logged_in && strcmp(clients[k].username, u->valuestring)==0) {
                                is_on=1; is_game=clients[k].is_busy; break;
                            }
                        }
                        LeaveCriticalSection(&cs_clients);
                        cJSON_AddStringToObject(pObj, "status", !is_on?"OFFLINE":(is_game?"IN_GAME":"FREE"));
                        cJSON_AddItemToArray(arr, pObj);
                    }
                }
                cJSON_Delete(accounts); free(file_content);
            }
            cJSON_AddItemToObject(res, "players", arr);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_INVITE_PLAYER) == 0) {
            cJSON *target = cJSON_GetObjectItem(req, "target");
            cJSON *num_q = cJSON_GetObjectItem(req, "num_questions");
            int n_q = num_q ? num_q->valueint : 5;
            
            EnterCriticalSection(&cs_clients);
            int t_idx = find_client_index(target->valuestring);
            if (t_idx != -1 && clients[t_idx].is_busy == 0) {
                char invite_info[100];
                sprintf(invite_info, "%s:%d", clients[client_index].username, n_q);
                strcpy(clients[t_idx].pending_invite_from, invite_info);
                clients[client_index].is_busy = 1; 
                cJSON_AddStringToObject(res, "type", "INVITE_SENT_SUCCESS");
            } else {
                cJSON_AddStringToObject(res, "type", MSG_TYPE_INVITE_FAIL);
                cJSON_AddStringToObject(res, "message", "Player busy or offline");
            }
            LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_ACCEPT_INVITE) == 0) {
            cJSON *inviter = cJSON_GetObjectItem(req, "from");
            EnterCriticalSection(&cs_clients);
            int i_idx = find_client_index(inviter->valuestring);
            if (i_idx != -1) {
                char *colon = strchr(clients[client_index].pending_invite_from, ':');
                int n_q = colon ? atoi(colon+1) : 5;
                
                int gid = create_game_session(clients[i_idx].username, clients[client_index].username, n_q);
                if (gid != -1) {
                    clients[client_index].is_busy = 1;
                    strcpy(clients[client_index].current_opponent, clients[i_idx].username);
                    strcpy(clients[client_index].pending_invite_from, "");
                    clients[client_index].game_session_id = gid;
                    clients[client_index].current_question_index = 0;

                    clients[i_idx].is_busy = 1;
                    strcpy(clients[i_idx].current_opponent, clients[client_index].username);
                    clients[i_idx].game_session_id = gid;
                    clients[i_idx].current_question_index = 0;
                    
                    EnterCriticalSection(&cs_lobby);
                    lobby_version++;
                    LeaveCriticalSection(&cs_lobby);
                    
                    cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_START);
                    cJSON_AddStringToObject(res, "opponent", clients[i_idx].username);
                    cJSON_AddNumberToObject(res, "total_questions", n_q);
                    cJSON_AddNumberToObject(res, "game_key", (double)game_sessions[gid].game_key);
                }
            }
            LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_REJECT_INVITE) == 0) {
            cJSON *inviter = cJSON_GetObjectItem(req, "from");
            EnterCriticalSection(&cs_clients);
            int i_idx = find_client_index(inviter->valuestring);
            strcpy(clients[client_index].pending_invite_from, "");
            if (i_idx != -1) {
                clients[i_idx].is_busy = 0;
                strcpy(clients[i_idx].current_opponent, "");
            }
            LeaveCriticalSection(&cs_clients);
            cJSON_AddStringToObject(res, "type", "REJECT_SUCCESS");
        }
        else if (strcmp(type->valuestring, MSG_TYPE_REQUEST_QUESTION) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            int gid = clients[client_index].game_session_id;
            int q_idx = clients[client_index].current_question_index;
            
            if (gid >= 0 && game_sessions[gid].is_active) {
                if (q_idx < game_sessions[gid].total_questions) {
                    int qid = game_sessions[gid].used_question_ids[q_idx];
                    char *content = read_file(QUESTION_FILE);
                    cJSON *qs = cJSON_Parse(content);
                    cJSON *q_found = NULL;
                    cJSON_ArrayForEach(q_found, qs) {
                        if (cJSON_GetObjectItem(q_found, "id")->valueint == qid) break;
                    }
                    if (q_found) {
                        cJSON_AddStringToObject(res, "type", MSG_TYPE_QUESTION);
                        cJSON_AddNumberToObject(res, "question_number", q_idx + 1);
                        cJSON_AddNumberToObject(res, "total_questions", game_sessions[gid].total_questions);
                        cJSON_AddStringToObject(res, "question", cJSON_GetObjectItem(q_found, "question")->valuestring);
                        cJSON_AddItemToObject(res, "options", cJSON_Duplicate(cJSON_GetObjectItem(q_found, "options"), 1));
                        cJSON_AddNumberToObject(res, "question_id", qid);
                        cJSON_AddNumberToObject(res, "max_time", MAX_TIME_PER_QUESTION);
                        
                        int is_p1 = (strcmp(clients[client_index].username, game_sessions[gid].player1)==0);
                        cJSON_AddNumberToObject(res, "opponent_score", is_p1 ? game_sessions[gid].score2 : game_sessions[gid].score1);
                    }
                    cJSON_Delete(qs); free(content);
                } else {
                    cJSON_AddStringToObject(res, "type", "NO_MORE_QUESTIONS");
                    int opp_idx = find_client_index(clients[client_index].current_opponent);
                    if (opp_idx != -1 && clients[opp_idx].current_question_index >= game_sessions[gid].total_questions) {
                        game_sessions[gid].is_active = 0;
                    }
                }
            } else {
                cJSON_AddStringToObject(res, "type", "ERROR");
                cJSON_AddStringToObject(res, "message", "Invalid Session");
            }
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_SUBMIT_ANSWER) == 0) {
            int qid = cJSON_GetObjectItem(req, "question_id")->valueint;
            int ans = cJSON_GetObjectItem(req, "answer_index")->valueint;
            // FIX 3: Doi ten bien time -> time_taken de tranh shadow ham time()
            double time_taken = cJSON_GetObjectItem(req, "time_taken")->valuedouble;
            
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            int gid = clients[client_index].game_session_id;
            if (gid >= 0 && game_sessions[gid].is_active) {
                char *content = read_file(QUESTION_FILE);
                cJSON *qs = cJSON_Parse(content);
                int correct = -1;
                cJSON *q = NULL;
                cJSON_ArrayForEach(q, qs) {
                    if (cJSON_GetObjectItem(q, "id")->valueint == qid) {
                        correct = cJSON_GetObjectItem(q, "answer_index")->valueint;
                        break;
                    }
                }
                cJSON_Delete(qs); free(content);
                
                int earned = calculate_score(ans == correct, time_taken);
                int is_p1 = (strcmp(clients[client_index].username, game_sessions[gid].player1) == 0);
                
                if(is_p1) {
                    game_sessions[gid].score1 += earned;
                    game_sessions[gid].player1_answers[clients[client_index].current_question_index] = ans;
                    game_sessions[gid].player1_times[clients[client_index].current_question_index] = time_taken;
                } else {
                    game_sessions[gid].score2 += earned;
                    game_sessions[gid].player2_answers[clients[client_index].current_question_index] = ans;
                    game_sessions[gid].player2_times[clients[client_index].current_question_index] = time_taken;
                }
                clients[client_index].current_question_index++;
                
                cJSON_AddStringToObject(res, "type", MSG_TYPE_ANSWER_RESULT);
                cJSON_AddBoolToObject(res, "is_correct", ans == correct);
                cJSON_AddNumberToObject(res, "correct_answer", correct);
                cJSON_AddNumberToObject(res, "earned_score", earned);
                cJSON_AddNumberToObject(res, "your_total_score", is_p1 ? game_sessions[gid].score1 : game_sessions[gid].score2);
                cJSON_AddNumberToObject(res, "opponent_score", is_p1 ? game_sessions[gid].score2 : game_sessions[gid].score1);
                
                if (clients[client_index].current_question_index >= game_sessions[gid].total_questions) {
                    int opp_idx = find_client_index(clients[client_index].current_opponent);
                    if (opp_idx != -1 && clients[opp_idx].current_question_index >= game_sessions[gid].total_questions) {
                        // Both finished
                        EnterCriticalSection(&cs_history);
                        if (history_count < MAX_HISTORY) {
                            game_history[history_count].game_key = game_sessions[gid].game_key;
                            strcpy(game_history[history_count].player1, game_sessions[gid].player1);
                            strcpy(game_history[history_count].player2, game_sessions[gid].player2);
                            game_history[history_count].score1 = game_sessions[gid].score1;
                            game_history[history_count].score2 = game_sessions[gid].score2;
                            game_history[history_count].total_questions = game_sessions[gid].total_questions;
                            // FIX 3: Gio ham time() se hoat dong dung
                            game_history[history_count].finished_time = time(NULL);
                            history_count++;
                        }
                        LeaveCriticalSection(&cs_history);
                        save_history_to_file();
                        
                        game_sessions[gid].is_active = 0;
                        cJSON_AddStringToObject(res, "game_status", "FINISHED");
                        cJSON_AddBoolToObject(res, "you_win", is_p1 ? (game_sessions[gid].score1 > game_sessions[gid].score2) : (game_sessions[gid].score2 > game_sessions[gid].score1));
                        
                        clients[client_index].is_busy = 0;
                        clients[client_index].game_session_id = -1;
                        memset(clients[client_index].current_opponent, 0, 50);
                    } else {
                        cJSON_AddStringToObject(res, "game_status", "WAITING_OPPONENT");
                    }
                } else {
                    cJSON_AddStringToObject(res, "game_status", "NEXT_QUESTION");
                }
            }
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_CHECK_GAME_STATUS) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            int gid = clients[client_index].game_session_id;
            if (gid >= 0) {
                int is_p1 = (strcmp(clients[client_index].username, game_sessions[gid].player1) == 0);
                int opp_idx = find_client_index(clients[client_index].current_opponent);
                
                int my_done = clients[client_index].current_question_index >= game_sessions[gid].total_questions;
                int opp_done = (opp_idx != -1) ? (clients[opp_idx].current_question_index >= game_sessions[gid].total_questions) : 1;
                
                if (!game_sessions[gid].is_active) opp_done = 1;
                
                cJSON_AddStringToObject(res, "type", MSG_TYPE_GAME_STATUS_UPDATE);
                cJSON_AddNumberToObject(res, "your_total_score", is_p1 ? game_sessions[gid].score1 : game_sessions[gid].score2);
                cJSON_AddNumberToObject(res, "opponent_score", is_p1 ? game_sessions[gid].score2 : game_sessions[gid].score1);
                
                if (my_done && opp_done) {
                    cJSON_AddStringToObject(res, "game_status", "FINISHED");
                    cJSON_AddBoolToObject(res, "you_win", is_p1 ? (game_sessions[gid].score1 > game_sessions[gid].score2) : (game_sessions[gid].score2 > game_sessions[gid].score1));
                    clients[client_index].is_busy = 0;
                    clients[client_index].game_session_id = -1;
                    clients[client_index].current_question_index = 0;
                    memset(clients[client_index].current_opponent, 0, 50);
                } else {
                    cJSON_AddStringToObject(res, "game_status", "WAITING_OPPONENT");
                }
            }
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_QUIT_GAME) == 0) {
            EnterCriticalSection(&cs_clients);
            EnterCriticalSection(&cs_games);
            int gid = clients[client_index].game_session_id;
            if (gid >= 0 && game_sessions[gid].is_active) {
                game_sessions[gid].is_active = 0;
                int opp_idx = find_client_index(clients[client_index].current_opponent);
                if (opp_idx != -1) {
                    clients[opp_idx].is_busy = 0;
                    clients[opp_idx].game_session_id = -1;
                    clients[opp_idx].current_question_index = 0;
                    memset(clients[opp_idx].current_opponent, 0, 50);
                }
            }
            clients[client_index].is_busy = 0;
            clients[client_index].current_question_index = 0;
            clients[client_index].game_session_id = -1;
            memset(clients[client_index].current_opponent, 0, 50);
            
            EnterCriticalSection(&cs_lobby);
            lobby_version++;
            LeaveCriticalSection(&cs_lobby);
            
            cJSON_AddStringToObject(res, "type", "QUIT_GAME_SUCCESS");
            LeaveCriticalSection(&cs_games);
            LeaveCriticalSection(&cs_clients);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_GET_HISTORY) == 0) {
            cJSON_AddStringToObject(res, "type", MSG_TYPE_HISTORY_DATA);
            cJSON *h_arr = cJSON_CreateArray();
            EnterCriticalSection(&cs_history);
            for (int i=0; i<history_count; i++) {
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
                    
                    int is_p1 = (strcmp(game_history[i].player1, clients[client_index].username)==0);
                    int my_s = is_p1 ? game_history[i].score1 : game_history[i].score2;
                    int opp_s = is_p1 ? game_history[i].score2 : game_history[i].score1;
                    cJSON_AddStringToObject(item, "result", (my_s > opp_s)?"WIN":(my_s < opp_s)?"LOSE":"DRAW");
                    cJSON_AddItemToArray(h_arr, item);
                }
            }
            LeaveCriticalSection(&cs_history);
            cJSON_AddItemToObject(res, "history", h_arr);
        }
        else if (strcmp(type->valuestring, MSG_TYPE_LOGOUT) == 0) {
            EnterCriticalSection(&cs_clients);
            if (clients[client_index].is_busy) {
                cJSON_AddStringToObject(res, "type", "LOGOUT_FAIL");
            } else {
                clients[client_index].is_logged_in = 0;
                EnterCriticalSection(&cs_lobby);
                lobby_version++;
                LeaveCriticalSection(&cs_lobby);
                cJSON_AddStringToObject(res, "type", MSG_TYPE_LOGOUT_SUCCESS);
            }
            LeaveCriticalSection(&cs_clients);
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