#include "auth.h"
#include <stdio.h>
#include <stdlib.h>

int register_user(const char* username, const char* password) {
    FILE* file;
    User user;
    
    printf("Registering user: %s\n", username);
    
    // Check if user exists
    file = fopen(USERS_FILE, "r");
    if (file != NULL) {
        while (fscanf(file, "%s %s", user.username, user.password) != EOF) {
            printf("Checking against existing user: %s\n", user.username);
            if (strcmp(user.username, username) == 0) {
                fclose(file);
                printf("User already exists: %s\n", username);
                return AUTH_USER_EXISTS;
            }
        }
        fclose(file);
    } else {
        printf("User file doesn't exist yet, will be created\n");
    }
    
    // Add new user
    file = fopen(USERS_FILE, "a");
    if (file == NULL) {
        printf("Failed to open users file for writing\n");
        return AUTH_FAILED;
    }
    
    fprintf(file, "%s %s\n", username, password);
    fclose(file);
    printf("User registered: %s\n", username);
    return AUTH_SUCCESS;
}

int authenticate_user(const char* username, const char* password) {
    FILE* file = fopen(USERS_FILE, "r");
    User user;
    
    printf("Authenticating user: %s\n", username);
    
    if (file == NULL) {
        printf("User file not found\n");
        return AUTH_FAILED;
    }
    
    while (fscanf(file, "%s %s", user.username, user.password) != EOF) {
        printf("Checking against: %s %s\n", user.username, user.password);
        if (strcmp(user.username, username) == 0 && 
            strcmp(user.password, password) == 0) {
            fclose(file);
            printf("Authentication successful for: %s\n", username);
            return AUTH_SUCCESS;
        }
    }
    
    fclose(file);
    printf("Authentication failed for: %s\n", username);
    return AUTH_FAILED;
}

int update_username(const char* old_username, const char* password, const char* new_username) {
    FILE* file = fopen(USERS_FILE, "r");
    FILE* temp = fopen("temp_users.txt", "w");
    User user;
    int found = 0;
    
    if (file == NULL || temp == NULL) {
        if (temp != NULL) fclose(temp);
        if (file != NULL) fclose(file);
        return AUTH_FAILED;
    }
    
    // Check if new username already exists
    rewind(file);
    while (fscanf(file, "%s %s", user.username, user.password) != EOF) {
        if (strcmp(user.username, new_username) == 0) {
            fclose(file);
            fclose(temp);
            remove("temp_users.txt");
            return AUTH_USER_EXISTS;
        }
    }
    
    // Update the username
    rewind(file);
    while (fscanf(file, "%s %s", user.username, user.password) != EOF) {
        if (strcmp(user.username, old_username) == 0 && 
            strcmp(user.password, password) == 0) {
            fprintf(temp, "%s %s\n", new_username, user.password);
            found = 1;
        } else {
            fprintf(temp, "%s %s\n", user.username, user.password);
        }
    }
    
    fclose(file);
    fclose(temp);
    
    if (!found) {
        remove("temp_users.txt");
        return AUTH_FAILED;
    }
    
    remove(USERS_FILE);
    rename("temp_users.txt", USERS_FILE);
    
    return AUTH_SUCCESS;
}

int update_password(const char* username, const char* old_password, const char* new_password) {
    FILE* file = fopen(USERS_FILE, "r");
    FILE* temp = fopen("temp_users.txt", "w");
    User user;
    int found = 0;
    
    if (file == NULL || temp == NULL) {
        if (temp != NULL) fclose(temp);
        if (file != NULL) fclose(file);
        return AUTH_FAILED;
    }
    
    while (fscanf(file, "%s %s", user.username, user.password) != EOF) {
        if (strcmp(user.username, username) == 0 && 
            strcmp(user.password, old_password) == 0) {
            fprintf(temp, "%s %s\n", user.username, new_password);
            found = 1;
        } else {
            fprintf(temp, "%s %s\n", user.username, user.password);
        }
    }
    
    fclose(file);
    fclose(temp);
    
    if (!found) {
        remove("temp_users.txt");
        return AUTH_FAILED;
    }
    
    remove(USERS_FILE);
    rename("temp_users.txt", USERS_FILE);
    
    return AUTH_SUCCESS;
}

int delete_account(const char* username, const char* password) {
    FILE* file = fopen(USERS_FILE, "r");
    FILE* temp = fopen("temp_users.txt", "w");
    User user;
    int found = 0;
    
    if (file == NULL || temp == NULL) {
        if (temp != NULL) fclose(temp);
        if (file != NULL) fclose(file);
        return AUTH_FAILED;
    }
    
    while (fscanf(file, "%s %s", user.username, user.password) != EOF) {
        if (strcmp(user.username, username) == 0 && 
            strcmp(user.password, password) == 0) {
            found = 1;
        } else {
            fprintf(temp, "%s %s\n", user.username, user.password);
        }
    }
    
    fclose(file);
    fclose(temp);
    
    if (!found) {
        remove("temp_users.txt");
        return AUTH_FAILED;
    }
    
    remove(USERS_FILE);
    rename("temp_users.txt", USERS_FILE);
    
    return AUTH_SUCCESS;
}
