#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/auth_service.h"
#include "../include/data_manager.h"

int process_register(const char* user, const char* pass) {
    if (check_username_exists(user)) {
        return 0; // Username already exists
    }
    
    char *file_content = read_file(ACCOUNT_FILE);
    cJSON *json = NULL;
    
    if (file_content) {
        json = cJSON_Parse(file_content);
        free(file_content);
    }
    
    if (!json || !cJSON_IsArray(json)) {
        if (json) cJSON_Delete(json);
        json = cJSON_CreateArray();
    }
    
    cJSON *new_account = cJSON_CreateObject();
    cJSON_AddStringToObject(new_account, "username", user);
    cJSON_AddStringToObject(new_account, "password", pass);
    cJSON_AddNumberToObject(new_account, "score", 0);
    
    cJSON_AddItemToArray(json, new_account);
    
    char *json_str = cJSON_Print(json);
    FILE *f = fopen(ACCOUNT_FILE, "w");
    int success = 0;
    
    if (f) {
        fprintf(f, "%s", json_str);
        fclose(f);
        success = 1;
        printf("[Auth] New account registered: %s\n", user);
    } else {
        printf("[Auth] ERROR: Could not save new account\n");
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