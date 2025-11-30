# Makefile cho Msys2/MinGW

CC = gcc
CFLAGS = -Wall
LIBS_SERVER = -lws2_32 -lpthread
LIBS_CLIENT = -lws2_32

all: server client_dll

# Bien dich Server
server: server.c cJSON.c protocol.h
	$(CC) $(CFLAGS) -o server server.c cJSON.c $(LIBS_SERVER)

# Bien dich Client DLL
client_dll: client_network.c cJSON.c protocol.h
	$(CC) $(CFLAGS) -shared -o client_network.dll client_network.c cJSON.c $(LIBS_CLIENT)

clean:
	rm -f server.exe client_network.dll