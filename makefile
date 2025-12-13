# Makefile pour Windows avec MinGW
CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lws2_32

all: server.exe client.exe

server.exe: server_windows.c
	$(CC) $(CFLAGS) -o server.exe server_windows.c $(LIBS)

client.exe: client_windows.c
	$(CC) $(CFLAGS) -o client.exe client_windows.c $(LIBS)

clean:
	del -f *.exe *.o server_log_windows.txt

run_server: server.exe
	server.exe

run_client: client.exe
	client.exe

debug_server: server_windows.c
	$(CC) $(CFLAGS) -g -o server_debug.exe server_windows.c $(LIBS)

debug_client: client_windows.c
	$(CC) $(CFLAGS) -g -o client_debug.exe client_windows.c $(LIBS)