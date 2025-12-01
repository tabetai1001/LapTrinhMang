// src/server/include/data_manager.h
#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "cJSON.h"

#define ACCOUNT_FILE "data/accounts.json"
#define QUESTION_FILE "data/questions.json"
#define HISTORY_FILE "data/history.json"

char* read_file(const char* filename);

// Các hàm mới
void load_questions_to_memory();
int update_user_score(const char* username, int score_to_add);

void save_history_to_file();
void load_history_from_file();
int check_username_exists(const char* username);

#endif