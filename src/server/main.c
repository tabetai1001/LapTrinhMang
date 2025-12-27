// src/server/main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "include/server_state.h"
#include "include/data_manager.h"
#include "include/connection_handler.h"

#define PORT 5555

int main() {
    int server_fd, *new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    pthread_t thread_id;

    printf("=== AI LA TRIEU PHU SERVER ===\n");

    // 1. Khởi tạo State và Data
    init_server_state();
    
    // Load dữ liệu vào RAM
    load_history_from_file();
    load_questions_to_memory(); // <--- QUAN TRỌNG: Load câu hỏi ở đây
    
    // 2. Tạo Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3. Bind & Listen
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }
    
    printf("[Server] Dang chay tai port %d...\n", PORT);

    // 4. Vòng lặp chấp nhận kết nối
    while (1) {
        new_socket = malloc(sizeof(int));
        *new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (*new_socket >= 0) {
            printf("[Connection] New client connected.\n");
            if (pthread_create(&thread_id, NULL, handle_client, (void *)new_socket) != 0) {
                perror("Thread creation failed");
                free(new_socket);
            }
            pthread_detach(thread_id);
        } else {
            free(new_socket);
        }
    }
    
    close(server_fd);
    return 0;
}