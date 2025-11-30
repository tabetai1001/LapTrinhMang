#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "cJSON.h"

#define ACCOUNT_FILE "data/accounts.json"
#define QUESTION_FILE "data/questions.json"
#define HISTORY_FILE "data/history.json"

char* read_file(const char* filename);
void save_history_to_file();
void load_history_from_file();
int check_username_exists(const char* username);

#endif