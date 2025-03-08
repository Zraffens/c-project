#ifndef AUTH_H
#define AUTH_H

#include <stdio.h>
#include <string.h>

#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32
#define USERS_FILE "users.txt"

typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
} User;

// Authentication results
#define AUTH_SUCCESS 0
#define AUTH_FAILED 1
#define AUTH_USER_EXISTS 2

int register_user(const char* username, const char* password);
int authenticate_user(const char* username, const char* password);
int update_username(const char* old_username, const char* password, const char* new_username);
int update_password(const char* username, const char* old_password, const char* new_password);
int delete_account(const char* username, const char* password);

#endif // AUTH_H
