/*
 * client.c
 *
 * A simple LAN-based messaging client using Winsock on Windows.
 * The client connects to the server using an IP address and port (provided as arguments),
 * then sends and receives plain text messages.
 *
 * To compile:
 *     gcc client.c -o client -lws2_32
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #include <windows.h>
 #include "common.h"
 
 #pragma comment(lib, "Ws2_32.lib")
 
 
 volatile BOOL client_running = TRUE;
 SOCKET connect_socket = INVALID_SOCKET;
 char current_username[32] = "";
 
 // Handler for Ctrl+C to allow graceful termination.
 BOOL WINAPI ConsoleHandler(DWORD signal) {
     if (signal == CTRL_C_EVENT) {
         printf("\nCtrl+C caught, shutting down client...\n");
         client_running = FALSE;
         closesocket(connect_socket);
     }
     return TRUE;
 }
 
 // Initialize Winsock.
 int initialize_winsock() {
     WSADATA wsaData;
     int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
     if (result != 0) {
         fprintf(stderr, "WSAStartup failed: %d\n", result);
         return 1;
     }
     return 0;
 }
 
 // Helper function to clear the console screen
 void clear_screen() {
     system("cls");
     printf("Chat cleared. You can continue typing.\n");
 }
 
 // Helper function to process commands
 int process_command(char* input, Message* msg) {
     char cmd[32];
     char args[BUFFER_SIZE];
     args[0] = '\0'; // Initialize args as empty string
     
     // Extract command and arguments
     if (sscanf(input, "/%s %[^\n]", cmd, args) < 1) {
         strcpy(cmd, "unknown");
     }
     
     msg->type = MSG_COMMAND;
     
     if (strcmp(cmd, "help") == 0) {
         msg->command = CMD_HELP;
         return 1;
     } 
     else if (strcmp(cmd, "username") == 0) {
         char new_username[32];
         char current_password[32];
         
         msg->command = CMD_USERNAME;
         
         if (sscanf(args, "%s", new_username) != 1) {
             printf("Usage: /username <new_username>\n");
             return 0;
         }
         
         // Prompt for the current password
         printf("Enter your current password: ");
         fgets(current_password, sizeof(current_password), stdin);
         current_password[strcspn(current_password, "\n")] = 0; // Remove newline
         
         // Format: "new_username current_password"
         snprintf(msg->content, BUFFER_SIZE, "%s %s", new_username, current_password);
         return 1;
     }
     else if (strcmp(cmd, "password") == 0) {
         char current_password[32];
         char new_password[32];
         
         msg->command = CMD_PASSWORD;
         
         // Prompt for the current password
         printf("Enter your current password: ");
         fgets(current_password, sizeof(current_password), stdin);
         current_password[strcspn(current_password, "\n")] = 0; // Remove newline
         
         // Prompt for the new password
         printf("Enter your new password: ");
         fgets(new_password, sizeof(new_password), stdin);
         new_password[strcspn(new_password, "\n")] = 0; // Remove newline
         
         // Format: "current_password new_password"
         snprintf(msg->content, BUFFER_SIZE, "%s %s", current_password, new_password);
         return 1;
     }
     else if (strcmp(cmd, "delete") == 0) {
         msg->command = CMD_DELETE;
         printf("Are you sure you want to delete your account? (y/n): ");
         char confirm;
         scanf(" %c", &confirm);
         getchar(); // Clear newline
         if (confirm == 'y' || confirm == 'Y') {
             printf("Enter your password to confirm: ");
             fgets(msg->content, BUFFER_SIZE, stdin);
             msg->content[strcspn(msg->content, "\n")] = 0;
             return 1;
         } else {
             printf("Account deletion cancelled.\n");
             return 0;
         }
     }
     else if (strcmp(cmd, "shout") == 0) {
         msg->command = CMD_SHOUT;
         strcpy(msg->content, args);
         return 1;
     }
     else if (strcmp(cmd, "whisper") == 0 || strcmp(cmd, "w") == 0) {
         char target[32];
         char whisper_msg[BUFFER_SIZE];
         
         if (sscanf(args, "%s %[^\n]", target, whisper_msg) == 2) {
             msg->command = CMD_WHISPER;
             strcpy(msg->target, target);
             strcpy(msg->content, whisper_msg);
             return 1;
         } else {
             printf("Usage: /whisper <username> <message>\n");
             return 0;
         }
     }
     else if (strcmp(cmd, "color") == 0) {
         msg->command = CMD_COLOR;
         
         if (strlen(args) == 0) {
             printf("Available colors: red, green, blue, yellow, magenta, cyan, white\n");
             printf("Usage: /color <colorname>\n");
             return 0;
         }
         
         strcpy(msg->content, args);
         return 1;
     }
     else if (strcmp(cmd, "roll") == 0) {
         msg->command = CMD_ROLL;
         return 1;
     }
     else if (strcmp(cmd, "online") == 0) {
         msg->command = CMD_ONLINE;
         return 1;
     }
     else if (strcmp(cmd, "clear") == 0) {
         msg->command = CMD_CLEAR;
         clear_screen();
         return 0; // No need to send to server
     }
     else if (strcmp(cmd, "joke") == 0) {
         msg->command = CMD_JOKE;
         return 1;
     }
     else {
         printf("Unknown command. Type /help for a list of commands.\n");
         return 0;
     }
 }
 
 // Thread function for receiving messages from the server.
 DWORD WINAPI receive_handler(LPVOID lpParam) {
     char buffer[BUFFER_SIZE];
     int recvResult;
     while (client_running) {
         ZeroMemory(buffer, BUFFER_SIZE);
         recvResult = recv(connect_socket, buffer, BUFFER_SIZE - 1, 0);
         if (recvResult > 0) {
             buffer[recvResult] = '\0';
             printf("%s\n", buffer);
         } else if (recvResult == 0) {
             printf("Server closed connection.\n");
             client_running = FALSE;
             break;
         } else {
             if (client_running) {
                 fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
             }
             client_running = FALSE;
             break;
         }
     }
     return 0;
 }
 
 // Function to handle authentication
 int authenticate(SOCKET socket) {
     Message msg, response;
     char username[32], password[32];
     int choice;
     
     printf("\n=== Authentication ===\n");
     printf("1. Login\n2. Register\nChoice: ");
     scanf("%d", &choice);
     getchar(); // Clear newline
     
     printf("Username: ");
     fgets(username, sizeof(username), stdin);
     username[strcspn(username, "\n")] = 0; // Remove newline
     
     printf("Password: ");
     fgets(password, sizeof(password), stdin);
     password[strcspn(password, "\n")] = 0; // Remove newline
     
     // Create authentication message
     ZeroMemory(&msg, sizeof(Message));
     msg.type = (choice == 1) ? MSG_AUTH : MSG_REGISTER;
     strcpy(msg.username, username);
     strcpy(msg.content, password);
     
     // Store the username if successful
     strcpy(current_username, username);
     
     printf("Sending authentication request type: %d for user: %s\n", msg.type, username);
     
     // Send authentication request
     if (send(socket, (char*)&msg, sizeof(Message), 0) == SOCKET_ERROR) {
         fprintf(stderr, "Authentication send failed: %d\n", WSAGetLastError());
         return 0;
     }
     
     // Wait for server response
     ZeroMemory(&response, sizeof(Message));
     
     int recvResult = recv(socket, (char*)&response, sizeof(Message), 0);
     if (recvResult > 0) {
         printf("Server response: %s\n", response.content);
         return (strstr(response.content, "successful") != NULL);
     } else {
         printf("Failed to receive server response\n");
     }
     
     return 0;
 }
 
 int main(int argc, char *argv[]) {
     if (argc < 3) {
         fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
         Sleep(5);
         return 1;
     }
     const char* serverIP = argv[1];
     const char* port = argv[2];
 
     // Set up the Ctrl+C handler.
     if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
         fprintf(stderr, "Could not set control handler\n");
         Sleep(5);
         return 1;
     }
 
     if (initialize_winsock() != 0) {
        printf("initialize");
        Sleep(5);
         return 1;
     }
 
     // Create a TCP socket.
     connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (connect_socket == INVALID_SOCKET) {
         fprintf(stderr, "socket failed: %ld\n", WSAGetLastError());
         Sleep(5);
         WSACleanup();
         return 1;
     }
 
     // Resolve the server address and port.
     struct addrinfo hints, *result;
     ZeroMemory(&hints, sizeof(hints));
     hints.ai_family = AF_INET;         // IPv4
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_protocol = IPPROTO_TCP;
 
     int res = getaddrinfo(serverIP, port, &hints, &result);
     if (res != 0) {
         fprintf(stderr, "getaddrinfo failed: %d\n", res);
         Sleep(5);
         closesocket(connect_socket);
         WSACleanup();
         return 1;
     }
 
     // Connect to the server.
     res = connect(connect_socket, result->ai_addr, (int)result->ai_addrlen);
     freeaddrinfo(result);
     if (res == SOCKET_ERROR) {
         fprintf(stderr, "connect failed: %d\n", WSAGetLastError());
         Sleep(5);
         
         closesocket(connect_socket);
         WSACleanup();
         return 1;
     }
     printf("Connected to server at %s:%s\n", serverIP, port);
 
     // Authenticate before chatting
     int authenticated = 0;
     while (!authenticated && client_running) {
         authenticated = authenticate(connect_socket);
         
         if (!authenticated) {
             printf("Authentication failed. Try again? (y/n): ");
             char retry;
             scanf(" %c", &retry);
             getchar(); // Clear newline
             
             if (retry != 'y') {
                 client_running = FALSE;
                 break;
             }
         }
     }
     
     // Only start receive thread after authentication
     HANDLE recvThread = NULL;
     if (authenticated) {
         clear_screen();
         printf("Authentication successful. You can now start chatting.\n");
         printf("Type /help to see available commands.\n\n");
         recvThread = CreateThread(NULL, 0, receive_handler, NULL, 0, NULL);
         if (recvThread == NULL) {
             fprintf(stderr, "Could not create receive thread.\n");
             closesocket(connect_socket);
             WSACleanup();
             return 1;
         }
         
         // Main loop for sending messages
         Message msg;
         char input[BUFFER_SIZE];
         while (client_running) {
             ZeroMemory(&msg, sizeof(Message));
             ZeroMemory(input, sizeof(input));
             
             if (fgets(input, BUFFER_SIZE, stdin) != NULL) {
                 input[strcspn(input, "\n")] = 0; // Remove newline
                 
                 if (strlen(input) > 0) {
                     // Check if it's a command
                     if (input[0] == '/') {
                         if (process_command(input, &msg)) {
                             if (send(connect_socket, (char*)&msg, sizeof(Message), 0) == SOCKET_ERROR) {
                                 fprintf(stderr, "Send failed: %d\n", WSAGetLastError());
                                 break;
                             }
                         }
                     } else {
                         // Regular chat message
                         msg.type = MSG_CHAT;
                         strcpy(msg.content, input);
                         
                         if (send(connect_socket, (char*)&msg, sizeof(Message), 0) == SOCKET_ERROR) {
                             fprintf(stderr, "Send failed: %d\n", WSAGetLastError());
                             break;
                         }
                         
                         // Display own message locally
                         printf("You: %s\n", input);
                     }
                 }
             }
         }
     }
     
     // Cleanup
     client_running = FALSE;
     if (recvThread != NULL) {
         WaitForSingleObject(recvThread, INFINITE);
         CloseHandle(recvThread);
     }
     closesocket(connect_socket);
     WSACleanup();
     return 0;
 }
