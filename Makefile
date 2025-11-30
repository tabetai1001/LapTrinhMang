# Makefile - Project Ai La Trieu Phu (Client-Server)
# Compiler: GCC
# OS: Windows (MinGW)

CC = gcc
# CFLAGS: 
# -Wall: Hien thi tat ca canh bao
# -I./src/common: Them thu muc common vao duong dan include (de tim thay protocol.h, cJSON.h)
CFLAGS = -Wall -I./src/common 

# LDFLAGS:
# -lws2_32: Link voi thu vien Winsock 2 (bat buoc cho lap trinh mang tren Windows)
LDFLAGS = -lws2_32

# Dinh nghia cac thu muc
SRC_COMMON = src/common
SRC_SERVER = src/server
SRC_CLIENT_NATIVE = src/client/native
BIN_DIR = bin

# --- TARGETS ---

# Target mac dinh: Tao thu muc bin, sau do build server va client dll
all: directories server client_dll

# 0. Tao thu muc bin neu chua ton tai
directories:
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)

# 1. Build Server (.exe)
# Server can: server.c va cJSON.c (de xu ly JSON)
server: $(SRC_SERVER)/server.c $(SRC_COMMON)/cJSON.c
	@echo [BUILD] Building Server...
	$(CC) $(CFLAGS) -o $(BIN_DIR)/server.exe $(SRC_SERVER)/server.c $(SRC_COMMON)/cJSON.c $(LDFLAGS)
	@echo [SUCCESS] Server built at $(BIN_DIR)/server.exe

# 2. Build Client Network DLL (.dll)
# Client can: client_network.c va cJSON.c (de dong goi vao DLL cho Python goi)
client_dll: $(SRC_CLIENT_NATIVE)/client_network.c $(SRC_COMMON)/cJSON.c
	@echo [BUILD] Building Client DLL...
	$(CC) $(CFLAGS) -shared -o $(BIN_DIR)/client_network.dll $(SRC_CLIENT_NATIVE)/client_network.c $(SRC_COMMON)/cJSON.c $(LDFLAGS)
	@echo [SUCCESS] Client DLL built at $(BIN_DIR)/client_network.dll

# 3. Don dep file da build
clean:
	@if exist $(BIN_DIR)\server.exe del $(BIN_DIR)\server.exe
	@if exist $(BIN_DIR)\client_network.dll del $(BIN_DIR)\client_network.dll
	@echo [CLEAN] Deleted binary files in $(BIN_DIR).

# 4. Chay thu Server (Luu y: phai chay tu thu muc goc de tim thay folder data)
run_server: server
	@echo [RUN] Starting Server...
	.\$(BIN_DIR)\server.exe