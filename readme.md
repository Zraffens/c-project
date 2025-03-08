# LAN Chat Application

A simple, multi-user chat application that allows communication over a Local Area Network (LAN).

## Features

- **Multi-user chat**: Multiple users can connect and chat simultaneously
- **User authentication**: Register and login with username/password
- **Private messaging**: Send private messages to specific users
- **Custom commands**: Various commands for different actions
- **Account management**: Change username, password, or delete account
- **Message formatting**: Shout messages, colored messages
- **Fun features**: Roll random numbers, tell jokes

## Building the Application

### Prerequisites

- Windows operating system
- GCC compiler (MinGW or MSYS2)
- Basic knowledge of command-line tools

### Compilation

1. Open a command prompt
2. Navigate to the project directory
3. Run the following command:
   ```
   mingw32-make all
   ```
4. This will create two executable files:
   - `server.exe`: The chat server
   - `client.exe`: The chat client

## Running the Application

### Starting the Server

1. Open a command prompt
2. Navigate to the project directory
3. Run the server:
   ```
   server.exe
   ```
   Optionally, specify a custom port:
   ```
   server.exe 9000
   ```
   (Default port is 8080)

4. The server will display: "Server: Listening on port 8080..."

### Connecting with a Client

1. Open a command prompt
2. Navigate to the project directory
3. Run the client with the server's IP address and port:
   ```
   client.exe 192.168.1.100 8080
   ```
   Replace the IP address with the server's actual IP on your network

4. Follow the prompts to register a new account or log in

## Using the Chat Application

### Authentication

When you first connect, you'll be prompted to:
1. Login (option 1) or Register (option 2)
2. Enter a username and password
   - Usernames must be at least 3 characters
   - Passwords must be at least 4 characters

### Chatting

- Type a message and press Enter to send it to all connected users
- Your own messages appear with "You:" prefix
- Messages from other users show their username
- System messages are prefixed with "[SYSTEM]"
- Private messages show "[PM from/to username]"

### Commands

The following commands are available by typing a slash (/) followed by the command name:

| Command | Description | Example |
|---------|-------------|---------|
| `/help` | Shows a list of all available commands | `/help` |
| `/username <new_name>` | Changes your username | `/username Alex` |
| `/password` | Changes your password | `/password` |
| `/delete` | Deletes your account | `/delete` |
| `/shout <message>` | Sends a message in ALL CAPS | `/shout Hello everyone!` |
| `/whisper <user> <message>` | Sends a private message to a user | `/whisper John Hi there!` |
| `/w <user> <message>` | Short version of whisper | `/w John Hi there!` |
| `/color <color>` | Changes your message color | `/color red` |
| `/roll` | Rolls a random number between 1-100 | `/roll` |
| `/online` | Shows a list of online users | `/online` |
| `/clear` | Clears your chat window | `/clear` |
| `/joke` | Tells a random joke to everyone | `/joke` |

#### Color Options

Available colors for the `/color` command:
- red
- green
- blue
- yellow
- magenta
- cyan
- white

## Account Management

### Changing Username

1. Type `/username <new_username>`
2. Enter your current password when prompted
3. If successful, your username will be changed, and others will be notified

### Changing Password

1. Type `/password`
2. Enter your current password when prompted
3. Enter your new password when prompted
4. If successful, your password will be updated

### Deleting Your Account

1. Type `/delete`
2. Confirm you want to delete your account by typing 'y'
3. Enter your password to confirm
4. If successful, your account will be deleted, and you'll be disconnected

## Troubleshooting

### Connection Issues

- Make sure the server is running before connecting with clients
- Check that you're using the correct IP address and port
- Ensure you're on the same local network
- Check if any firewall is blocking the connection

### Authentication Problems

- Usernames and passwords are case-sensitive
- If you forget your password, there's currently no password recovery (the server administrator can manually edit the users.txt file)

### Command Not Working

- Make sure to type the command exactly as shown (with the leading slash)
- Check the command syntax in the help menu (`/help`)
- Some commands require additional input or confirmation

## Exiting the Application

- Press Ctrl+C to exit either the client or server
- The application will clean up resources and close properly