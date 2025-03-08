CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lws2_32

all: server.exe client.exe

server.exe: server.c auth.c common.h auth.h
	$(CC) $(CFLAGS) server.c auth.c -o server.exe $(LIBS)

client.exe: client.c common.h
	$(CC) $(CFLAGS) client.c -o client.exe $(LIBS)

clean:
	del server.exe client.exe
