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

    // 1. Khởi tạo State và Data
    init_server_state();
    load_history_from_file();
    
    // 2. Khởi tạo Winsock
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 3. Tạo Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 4. Bind & Listen
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("[Server] Dang chay tai port %d...\n", PORT);

    // 5. Vòng lặp chấp nhận kết nối
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket != INVALID_SOCKET) {
            CreateThread(NULL, 0, handle_client, (LPVOID)new_socket, 0, NULL);
        }
    }
    return 0;
}