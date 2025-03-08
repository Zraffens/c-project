#ifndef COMMON_H
#define COMMON_H

#include <winsock2.h>       // Must come before windows.h
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT "8080"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Message types
#define MSG_AUTH 1
#define MSG_REGISTER 2
#define MSG_CHAT 3
#define MSG_COMMAND 4
#define MSG_SYSTEM 5
#define MSG_PRIVATE 6

// Command types
#define CMD_HELP 1
#define CMD_USERNAME 2
#define CMD_PASSWORD 3
#define CMD_DELETE 4
#define CMD_SHOUT 5
#define CMD_WHISPER 6
#define CMD_COLOR 7
#define CMD_ROLL 8
#define CMD_ONLINE 9
#define CMD_CLEAR 10
#define CMD_JOKE 11
#define CMD_UNKNOWN 99

// Message structure
typedef struct {
    int type;
    int command;
    char username[32];
    char target[32];  // For whisper command
    char content[BUFFER_SIZE];
} Message;

// Client structure
typedef struct {
    SOCKET socket;
    int id;
    char username[32];
    int authenticated;
    char color[10];  // Color for messages
} Client;

#endif // COMMON_H
