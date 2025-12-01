// src/server/main.c
#include <stdio.h>
#include <winsock2.h>
#include "include/server_state.h"
#include "include/data_manager.h"
#include "include/connection_handler.h"

#define PORT 5555

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    printf("=== AI LA TRIEU PHU SERVER ===\n");

    // 1. Khởi tạo State và Data
    init_server_state();
    
    // Load dữ liệu vào RAM
    load_history_from_file();
    load_questions_to_memory(); // <--- QUAN TRỌNG: Load câu hỏi ở đây
    
    // 2. Khởi tạo Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // 3. Tạo Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 4. Bind & Listen
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        return 1;
    }
    
    listen(server_fd, 3);
    printf("[Server] Dang chay tai port %d...\n", PORT);

    // 5. Vòng lặp chấp nhận kết nối
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket != INVALID_SOCKET) {
            printf("[Connection] New client connected.\n");
            CreateThread(NULL, 0, handle_client, (LPVOID)new_socket, 0, NULL);
        }
    }
    
    return 0;
}