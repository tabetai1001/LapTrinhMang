#ifndef GAME_SERVICE_H
#define GAME_SERVICE_H

#include "cJSON.h"
#include "models.h"

int create_game_session(const char* p1, const char* p2, int num_questions);
cJSON* get_random_question(int *used_ids, int used_count);
int calculate_score(int is_correct, double time_taken);
cJSON* get_leaderboard_json();
void broadcast_lobby_update();
int find_client_index(const char* username);
int compare_scores(const void *a, const void *b);
cJSON* process_lifeline(int game_id, const char* username, int lifeline_id);

#endif