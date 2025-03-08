# Chat Application Core Algorithms

This document outlines the key algorithms in the LAN Chat application.

## Authentication System

### User Registration

Algorithm

RegisterUser(username, password): 1. Check if username exists in user database 2. If exists, return "username already exists" error 3. If not, add new username/password pair to database 4. Return success

### User Authentication

Algorithm AuthenticateUser(username, password): 1. Search user database for username 2. If found and password matches, return success 3. Otherwise, return authentication failed

## Server Core Algorithms

### Server Main Loop

Algorithm ServerMainLoop: 1. Initialize Winsock and create listening socket 2. While server is running: a. Accept new client connection b. Create new client structure c. Start new thread for client handling 3. On shutdown, close all connections and release resources

### Client Handler

Algorithm HandleClient(client): 1. Initialize client as unauthenticated 2. While connection active: a. Receive message from client b. If client not authenticated: - Allow only AUTH/REGISTER messages - Reject all other message types c. Process message based on type: - Authentication: Validate credentials - Registration: Create new account - Chat: Broadcast to other clients - Command: Execute special action 3. On disconnect, remove client from active clients list

### Message Broadcasting

Algorithm BroadcastMessage(sender, message): 1. For each connected client except sender: a. Send message to client

## Client Algorithms

### Client Initialization

Algorithm InitializeClient: 1. Connect to server using provided IP and port 2. Authenticate with username and password 3. If authenticated: a. Start receive thread b. Enter main message loop 4. On exit, close connection and clean up resources

### Message Processing

Algorithm ProcessUserInput: 1. Read input from user 2. If input starts with '/': a. Parse as command b. Process locally or send to server 3. Otherwise: a. Send as regular chat message b. Display locally as "You: [message]"

## Command System

### Command Processing

Algorithm ProcessCommand(command): 1. Parse command name and arguments 2. Based on command name: - help: Display available commands - username: Change username with password verification - password: Change password - delete: Delete account with confirmation - shout: Send uppercase message to all users - whisper: Send private message to specific user - color: Change message color - roll: Generate random number - online: List connected users - clear: Clear screen (client-side only) - joke: Send random joke to all users 3. Execute appropriate action for command

## Thread Management

### Thread Synchronization

Algorithm AccessSharedResource: 1. Enter critical section (lock) 2. Access or modify shared resource 3. Leave critical section (unlock)
