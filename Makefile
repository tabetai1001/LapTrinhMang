# Makefile - Project Ai La Trieu Phu (Modular Version)
# Compiler: GCC
# OS: Windows (MinGW)

CC = gcc

# CFLAGS: 
# -Wall: Hien thi canh bao
# -I...: Them cac thu muc header vao duong dan include
CFLAGS = -Wall -I./src/common -I./src/server/include 

# LDFLAGS:
# -lws2_32: Link thu vien Winsock 2
LDFLAGS = -lws2_32

# --- DUONG DAN THU MUC ---
BIN_DIR = bin
SRC_COMMON = src/common
SRC_SERVER_DIR = src/server
SRC_SERVER_MODULES = src/server/modules
SRC_CLIENT_NATIVE = src/client/native

# --- DANH SACH FILE NGUON SERVER ---
# Bao gom main.c, cac modules chuc nang va cJSON
SERVER_SOURCES = $(SRC_SERVER_DIR)/main.c \
                 $(SRC_SERVER_MODULES)/server_state.c \
                 $(SRC_SERVER_MODULES)/data_manager.c \
                 $(SRC_SERVER_MODULES)/auth_service.c \
                 $(SRC_SERVER_MODULES)/game_service.c \
                 $(SRC_SERVER_MODULES)/connection_handler.c \
                 $(SRC_COMMON)/cJSON.c

# --- DANH SACH FILE NGUON CLIENT DLL ---
# Bao gom client_network.c va cJSON
CLIENT_SOURCES = $(SRC_CLIENT_NATIVE)/client_network.c \
                 $(SRC_COMMON)/cJSON.c

# --- TARGETS ---

# 1. Target mac dinh: Tao bin -> Build Server -> Build Client DLL
all: directories server client_dll

# 2. Tao thu muc bin neu chua co
directories:
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)

# 3. Build Server (.exe)
server: $(SERVER_SOURCES)
	@echo [BUILD] Building Server (Modular)...
	$(CC) $(CFLAGS) -o $(BIN_DIR)/server.exe $(SERVER_SOURCES) $(LDFLAGS)
	@echo [SUCCESS] Server built at $(BIN_DIR)/server.exe

# 4. Build Client Network DLL (.dll)
client_dll: $(CLIENT_SOURCES)
	@echo [BUILD] Building Client DLL...
	$(CC) $(CFLAGS) -shared -o $(BIN_DIR)/client_network.dll $(CLIENT_SOURCES) $(LDFLAGS)
	@echo [SUCCESS] Client DLL built at $(BIN_DIR)/client_network.dll

# 5. Don dep (Xoa file da build)
clean:
	@if exist $(BIN_DIR)\server.exe del $(BIN_DIR)\server.exe
	@if exist $(BIN_DIR)\client_network.dll del $(BIN_DIR)\client_network.dll
	@echo [CLEAN] Cleanup complete.

# 6. Chay thu Server
run: server
	@echo [RUN] Starting Server...
	.\$(BIN_DIR)\server.exe