#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10
#define PORT 9002

struct sockaddr_in server_address = {
    .sin_family         = AF_INET,
    .sin_port           = htons(PORT),
    .sin_addr.s_addr    = INADDR_ANY
};

int create_server_socket() {
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}
#endif

