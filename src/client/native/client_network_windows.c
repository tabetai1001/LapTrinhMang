/*
 * client_network_windows.c - Windows Version
 * Compile on Windows: gcc -shared -o client_network.dll client_network_windows.c ../../common/cJSON.c -lws2_32
 */

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define EXPORT __declspec(dllexport)
#define BUFFER_SIZE 4096

SOCKET client_socket = INVALID_SOCKET;
char response_buffer[BUFFER_SIZE]; 

// Hàm khởi tạo Winsock
void init_ws() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
}

EXPORT int connect_to_server(const char* ip, int port) {
    if (client_socket == INVALID_SOCKET) init_ws();
    
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return 0;
    }
    return 1;
}

EXPORT const char* send_request_and_wait(const char* json_req) {
    if (client_socket == INVALID_SOCKET) return "{\"type\":\"ERROR\"}";

    // 1. Gửi dữ liệu
    send(client_socket, json_req, strlen(json_req), 0);

    // 2. Nhận dữ liệu
    int valread = recv(client_socket, response_buffer, BUFFER_SIZE - 1, 0);
    
    if (valread > 0) {
        // BUG FIX: Thêm ký tự kết thúc chuỗi để Python không đọc rác
        response_buffer[valread] = '\0'; 
        return response_buffer;
    }
    return "{\"type\":\"ERROR\"}";
}

EXPORT void close_connection() {
    closesocket(client_socket);
    WSACleanup();
}
