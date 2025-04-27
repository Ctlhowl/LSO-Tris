#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int create_socket() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket creation failed");
        return -1;
    }
    return socket_fd;
}

void setup_address(struct sockaddr_in* address, int port) {
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(port);
}

int enable_socket_options(int socket_fd) {
    int opt = 1;
    // Permette il riutilizzo dell'indirizzo
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -1;
    }
    
    return 0;
}

int bind_and_listen(int socket_fd, struct sockaddr_in* address) {
    if (bind(socket_fd, (struct sockaddr*)address, sizeof(*address)) < 0) {
        perror("bind failed");
        return -1;
    }

    if (listen(socket_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        return -1;
    }

    printf("Server listening on port %d\n", ntohs(address->sin_port));
    return 0;
}