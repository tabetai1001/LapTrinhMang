#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <pthread.h>

void* handle_client(void* client_socket_ptr);

#endif