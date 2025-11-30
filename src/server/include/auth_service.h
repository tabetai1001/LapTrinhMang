#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "cJSON.h"

int process_register(const char* user, const char* pass);
int process_login(const char* user, const char* pass, int* out_score);

#endif