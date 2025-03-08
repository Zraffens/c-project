/*
* server.c
*
* A simple LAN-based messaging server using Winsock on Windows.
* It listens on a configurable port (default: 8080) and relays plain text messages
* received from any connected client to all other clients.
*
* To compile:
*     gcc server.c auth.c -o server -lws2_32
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <time.h>     // Add time.h for time() function
#include "auth.h"
#include "common.h"

// Remove pragma comment - we're using -lws2_32 in the Makefile

// Structure to represent a connected client.


// Global array to hold pointers to connected clients.
Client *clients[MAX_CLIENTS] = { 0 };
CRITICAL_SECTION clients_mutex;  // To synchronize access to the clients array.
volatile BOOL server_running = TRUE;  // Flag to control the server loop.

// Handler for Ctrl+C (SIGINT) to allow graceful shutdown.
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        printf("\nCtrl+C caught, shutting down server...\n");
        server_running = FALSE;
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

// Create, bind, and listen on a socket for the given port.
SOCKET create_listening_socket(const char* port) {
    struct addrinfo hints, *serverInfo, *p;
    SOCKET listen_socket = INVALID_SOCKET;
    int result;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;         // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;        // Use local IP

    result = getaddrinfo(NULL, port, &hints, &serverInfo);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo failed: %d\n", result);
        return INVALID_SOCKET;
    }

    // Loop through all results and bind to the first we can.
    for (p = serverInfo; p != NULL; p = p->ai_next) {
        listen_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_socket == INVALID_SOCKET) {
            fprintf(stderr, "socket failed: %d\n", WSAGetLastError()); // Changed %ld to %d
            continue;
        }

        result = bind(listen_socket, p->ai_addr, (int)p->ai_addrlen);
        if (result == SOCKET_ERROR) {
            fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
            closesocket(listen_socket);
            listen_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(serverInfo);

    if (listen_socket == INVALID_SOCKET) {
        fprintf(stderr, "Error: Unable to bind to port %s\n", port);
        return INVALID_SOCKET;
    }

    result = listen(listen_socket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }

    return listen_socket;
}

// Broadcast a message to all clients except the sender.
void broadcast_message(int sender_id, const char* message) {
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->socket != INVALID_SOCKET) {
            // Optionally, skip sending back to the sender.
            if (clients[i]->id == sender_id)
                continue;
            int sendResult = send(clients[i]->socket, message, (int)strlen(message), 0);
            if (sendResult == SOCKET_ERROR) {
                fprintf(stderr, "send failed to client %d: %d\n", clients[i]->id, WSAGetLastError());
            }
        }
    }
    LeaveCriticalSection(&clients_mutex);
}

// Find client by username
Client* find_client_by_username(const char* username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->authenticated && 
            strcmp(clients[i]->username, username) == 0) {
            return clients[i];
        }
    }
    return NULL;
}

// Send system message to a client
void send_system_message(Client* client, const char* message) {
    char system_msg[BUFFER_SIZE];
    snprintf(system_msg, BUFFER_SIZE, "[SYSTEM] %s", message);
    send(client->socket, system_msg, strlen(system_msg), 0);
}

// Send private message
void send_private_message(Client* sender, Client* receiver, const char* message) {
    char private_msg[BUFFER_SIZE];
    
    // Format for receiver
    snprintf(private_msg, BUFFER_SIZE, "[PM from %s] %s", sender->username, message);
    send(receiver->socket, private_msg, strlen(private_msg), 0);
    
    // Format for sender (confirmation)
    snprintf(private_msg, BUFFER_SIZE, "[PM to %s] %s", receiver->username, message);
    send(sender->socket, private_msg, strlen(private_msg), 0);
}

// Get a list of online users
void get_online_users(char* buffer) {
    int count = 0;
    strcpy(buffer, "Online users: ");
    
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->authenticated) {
            count++;
            strcat(buffer, clients[i]->username);
            strcat(buffer, ", ");
        }
    }
    LeaveCriticalSection(&clients_mutex);
    
    // Remove the trailing comma and space
    if (count > 0) {
        buffer[strlen(buffer) - 2] = '\0';
    } else {
        strcat(buffer, "No users online");
    }
}

// Get a random joke
void get_random_joke(char* buffer, const char* username) {
    const char* jokes[] = {
        "Why don't scientists trust atoms? Because they make up everything!",
        "Did you hear about the mathematician who's afraid of negative numbers? He'll stop at nothing to avoid them!",
        "Why was the computer cold? It left its Windows open!",
        "What's a programmer's favorite place? The Foo Bar!",
        "How many programmers does it take to change a light bulb? None, it's a hardware problem!",
        "There are 10 kinds of people in this world: those who understand binary and those who don't.",
        "Why do programmers prefer dark mode? Because light attracts bugs!"
    };
    
    int joke_count = sizeof(jokes) / sizeof(jokes[0]);
    int random_index = rand() % joke_count;
    
    snprintf(buffer, BUFFER_SIZE, "[JOKE from %s] %s", username, jokes[random_index]);
}

// Apply ANSI color to a message
void apply_color(char* colored_msg, const char* original_msg, const char* color, int max_size) {
    const char* color_code;
    
    if (strcmp(color, "red") == 0) color_code = "\033[31m";
    else if (strcmp(color, "green") == 0) color_code = "\033[32m";
    else if (strcmp(color, "yellow") == 0) color_code = "\033[33m";
    else if (strcmp(color, "blue") == 0) color_code = "\033[34m";
    else if (strcmp(color, "magenta") == 0) color_code = "\033[35m";
    else if (strcmp(color, "cyan") == 0) color_code = "\033[36m";
    else color_code = "\033[0m"; // default/white
    
    snprintf(colored_msg, max_size, "%s%s\033[0m", color_code, original_msg);
}

// Process commands from clients
void process_command(Client* client, Message* msg) {
    char response[BUFFER_SIZE];
    
    switch (msg->command) {
        case CMD_HELP:
            snprintf(response, BUFFER_SIZE, 
                "Available commands:\n"
                "/help - Show this help message\n"
                "/username <new_username> - Change your username\n"
                "/password <new_password> - Change your password\n"
                "/delete - Delete your account\n"
                "/shout <message> - Send a message in UPPERCASE\n"
                "/whisper <username> <message> - Send a private message\n"
                "/w <username> <message> - Shorthand for whisper\n"
                "/color <color> - Change your message color (not implemented in console)\n"
                "/roll - Roll a random number\n"
                "/online - Show all online users\n"
                "/clear - Clear the chat window\n"
                "/joke - Tell a random joke"
            );
            send_system_message(client, response);
            break;
            
        case CMD_USERNAME:
            {
                char new_username[32];
                char current_password[32];
                
                // Split the content into new_username and current_password
                // Format expected: "new_username current_password"
                if (sscanf(msg->content, "%s %s", new_username, current_password) != 2) {
                    send_system_message(client, "Usage: /username <new_username> <current_password>");
                    break;
                }
                
                if (strlen(new_username) < 3) {
                    send_system_message(client, "Username must be at least 3 characters");
                    break;
                }
                
                // Check if name is taken by another online user
                if (find_client_by_username(new_username) != NULL) {
                    send_system_message(client, "Username already taken");
                    break;
                }
                
                // Verify the current password before changing the username
                if (authenticate_user(client->username, current_password) != AUTH_SUCCESS) {
                    send_system_message(client, "Current password is incorrect");
                    break;
                }
                
                int result = update_username(client->username, current_password, new_username);
                
                if (result == AUTH_SUCCESS) {
                    snprintf(response, BUFFER_SIZE, "Username changed from %s to %s", client->username, new_username);
                    send_system_message(client, response);
                    
                    // Broadcast the name change
                    snprintf(response, BUFFER_SIZE, "User %s is now known as %s", client->username, new_username);
                    broadcast_message(-1, response);
                    
                    // Update client's username
                    strcpy(client->username, new_username);
                } else if (result == AUTH_USER_EXISTS) {
                    send_system_message(client, "Username already exists");
                } else {
                    send_system_message(client, "Failed to change username");
                }
            }
            break;
            
        case CMD_PASSWORD:
            {
                char new_password[32];
                char current_password[32];
                
                // Split the content into current_password and new_password
                // Format expected: "current_password new_password"
                if (sscanf(msg->content, "%s %s", current_password, new_password) != 2) {
                    send_system_message(client, "Usage: /password <current_password> <new_password>");
                    break;
                }
                
                if (strlen(new_password) < 4) {
                    send_system_message(client, "New password must be at least 4 characters");
                    break;
                }
                
                int result = update_password(client->username, current_password, new_password);
                
                if (result == AUTH_SUCCESS) {
                    send_system_message(client, "Password changed successfully");
                } else {
                    send_system_message(client, "Failed to change password. Check your current password.");
                }
            }
            break;
            
        case CMD_DELETE:
            {
                char password[32];
                strcpy(password, msg->content);
                
                int result = delete_account(client->username, password);
                
                if (result == AUTH_SUCCESS) {
                    send_system_message(client, "Your account has been deleted. You will be disconnected.");
                    // Force disconnect
                    client->authenticated = 0;
                } else {
                    send_system_message(client, "Failed to delete account. Check your password.");
                }
            }
            break;
            
        case CMD_SHOUT:
            {
                char shout_msg[BUFFER_SIZE];
                // Convert message to uppercase
                strncpy(shout_msg, msg->content, BUFFER_SIZE-1);
                shout_msg[BUFFER_SIZE-1] = '\0';
                for (int i = 0; shout_msg[i]; i++) {
                    shout_msg[i] = toupper((unsigned char)shout_msg[i]);
                }
                
                // Format the message
                snprintf(response, BUFFER_SIZE, "%s SHOUTS: %s", client->username, shout_msg);
                broadcast_message(-1, response);
                
                // Send back to the sender too
                send(client->socket, response, strlen(response), 0);
            }
            break;
            
        case CMD_WHISPER:
            {
                Client* target = find_client_by_username(msg->target);
                
                if (target != NULL) {
                    send_private_message(client, target, msg->content);
                } else {
                    sprintf(response, "User '%s' is not online", msg->target);
                    send_system_message(client, response);
                }
            }
            break;
            
        case CMD_COLOR:
            {
                strncpy(client->color, msg->content, 9);
                client->color[9] = '\0'; // Ensure null-termination
                
                // List of supported colors
                const char* supported_colors = "red, green, blue, yellow, magenta, cyan, white";
                
                snprintf(response, BUFFER_SIZE, 
                    "Your message color has been set to %s. Supported colors: %s", 
                    client->color, supported_colors);
                send_system_message(client, response);
                
                // Send a sample colored message
                char colored_msg[BUFFER_SIZE];
                snprintf(response, BUFFER_SIZE, "This is a sample message in your chosen color.");
                apply_color(colored_msg, response, client->color, BUFFER_SIZE);
                send(client->socket, colored_msg, strlen(colored_msg), 0);
            }
            break;
            
        case CMD_ROLL:
            {
                int roll = rand() % 100 + 1;
                snprintf(response, BUFFER_SIZE, "%s rolled %d (1-100)", client->username, roll);
                broadcast_message(-1, response);
            }
            break;
            
        case CMD_ONLINE:
            {
                get_online_users(response);
                send_system_message(client, response);
            }
            break;
            
        case CMD_JOKE:
            {
                get_random_joke(response, client->username);
                broadcast_message(-1, response);
                send(client->socket, response, strlen(response), 0);
            }
            break;
            
        default:
            send_system_message(client, "Unknown command. Type /help for a list of commands.");
            break;
    }
}

// Thread function to handle an individual client.
DWORD WINAPI handle_client(LPVOID lpParam) {
    Client* client = (Client*)lpParam;
    Message msg, response;
    int recvResult;
    client->authenticated = 0;
    strcpy(client->username, "");
    strcpy(client->color, "default"); // Default message color

    printf("Client %d connected.\n", client->id);
    while (server_running) {
        ZeroMemory(&msg, sizeof(Message));
        recvResult = recv(client->socket, (char*)&msg, sizeof(Message), 0);
        
        if (recvResult > 0) {
            printf("Received message type: %d from client %d\n", msg.type, client->id);
            
            // Skip commands from unauthenticated clients, except auth commands
            if (!client->authenticated && msg.type != MSG_AUTH && msg.type != MSG_REGISTER) {
                ZeroMemory(&response, sizeof(Message));
                strcpy(response.content, "Please login first");
                send(client->socket, (char*)&response, sizeof(Message), 0);
                continue;
            }

            switch (msg.type) {
                case MSG_AUTH:
                    printf("Auth attempt with username: %s\n", msg.username);
                    ZeroMemory(&response, sizeof(Message));
                    
                    if (authenticate_user(msg.username, msg.content) == AUTH_SUCCESS) {
                        strcpy(client->username, msg.username);
                        client->authenticated = 1;
                        strcpy(response.content, "Login successful");
                        send(client->socket, (char*)&response, sizeof(Message), 0);
                        printf("Client %d authenticated as %s\n", client->id, client->username);
                    } else {
                        strcpy(response.content, "Login failed");
                        send(client->socket, (char*)&response, sizeof(Message), 0);
                        printf("Authentication failed for username: %s\n", msg.username);
                    }
                    break;

                case MSG_REGISTER:
                    printf("Registration attempt for username: %s\n", msg.username);
                    ZeroMemory(&response, sizeof(Message));
                    
                    int regResult = register_user(msg.username, msg.content);
                    if (regResult == AUTH_SUCCESS) {
                        strcpy(response.content, "Registration successful");
                        send(client->socket, (char*)&response, sizeof(Message), 0);
                        printf("New user registered: %s\n", msg.username);
                    } else if (regResult == AUTH_USER_EXISTS) {
                        strcpy(response.content, "Username already exists");
                        send(client->socket, (char*)&response, sizeof(Message), 0);
                        printf("Registration failed - username exists: %s\n", msg.username);
                    } else {
                        strcpy(response.content, "Registration failed");
                        send(client->socket, (char*)&response, sizeof(Message), 0);
                        printf("Registration failed for username: %s\n", msg.username);
                    }
                    break;

                case MSG_CHAT:
                    if (client->authenticated) {
                        char formatted_msg[BUFFER_SIZE + 50];
                        char colored_msg[BUFFER_SIZE + 100];  // Extra space for color codes
                        
                        snprintf(formatted_msg, sizeof(formatted_msg), "%s: %s", client->username, msg.content);
                        printf("%s\n", formatted_msg);
                        
                        // Apply the client's preferred color if set
                        if (strcmp(client->color, "default") != 0) {
                            apply_color(colored_msg, formatted_msg, client->color, sizeof(colored_msg));
                            broadcast_message(client->id, colored_msg);
                        } else {
                            broadcast_message(client->id, formatted_msg);
                        }
                    }
                    break;
                
                case MSG_COMMAND:
                    if (client->authenticated) {
                        process_command(client, &msg);
                    }
                    break;
                
                default:
                    // Unknown message type
                    printf("Unknown message type: %d\n", msg.type);
                    break;
            }
        } else if (recvResult == 0) {
            // Connection closed by client.
            printf("Client %d disconnected.\n", client->id);
            break;
        } else {
            fprintf(stderr, "recv failed from client %d: %d\n", client->id, WSAGetLastError());
            break;
        }
    }

    // Remove the client from the global list.
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client) {
            clients[i] = NULL;
            break;
        }
    }
    LeaveCriticalSection(&clients_mutex);

    closesocket(client->socket);
    free(client);
    return 0;
}

int main(int argc, char *argv[]) {
    const char* port = DEFAULT_PORT;
    if (argc > 1) {
        port = argv[1];  // Use port provided as argument.
    }

    // Seed random number generator for dice rolls and jokes
    srand(time(NULL));

    // Set up the Ctrl+C handler.
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        fprintf(stderr, "Could not set control handler\n");
        return 1;
    }

    if (initialize_winsock() != 0) {
        return 1;
    }

    SOCKET listen_socket = create_listening_socket(port);
    if (listen_socket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // Initialize critical section for managing client list.
    InitializeCriticalSection(&clients_mutex);

    printf("Server: Listening on port %s...\n", port);

    // Main loop: accept new client connections.
    while (server_running) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            if (server_running) {
                fprintf(stderr, "accept failed: %d\n", WSAGetLastError());
            }
            break;
        }

        // Allocate a new Client structure.
        Client* client = (Client*)malloc(sizeof(Client));
        if (client == NULL) {
            fprintf(stderr, "Memory allocation failed for client.\n");
            closesocket(client_socket);
            continue;
        }
        client->socket = client_socket;

        // Assign a client id and add the client to the global array.
        EnterCriticalSection(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == NULL) {
                client->id = i + 1; // IDs start at 1.
                clients[i] = client;
                break;
            }
        }
        LeaveCriticalSection(&clients_mutex);

        // Create a new thread to handle the client.
        HANDLE threadHandle = CreateThread(NULL, 0, handle_client, (LPVOID)client, 0, NULL);
        if (threadHandle == NULL) {
            fprintf(stderr, "Could not create thread for client %d\n", client->id);
            EnterCriticalSection(&clients_mutex);
            // Remove client from list.
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == client) {
                    clients[i] = NULL;
                    break;
                }
            }
            LeaveCriticalSection(&clients_mutex);
            closesocket(client_socket);
            free(client);
        } else {
            CloseHandle(threadHandle); // No need to keep the thread handle.
        }
    }

    // Cleanup: close all client sockets.
    printf("Server shutting down...\n");
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL) {
            closesocket(clients[i]->socket);
            free(clients[i]);
            clients[i] = NULL;
        }
    }
    LeaveCriticalSection(&clients_mutex);

    DeleteCriticalSection(&clients_mutex);
    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
