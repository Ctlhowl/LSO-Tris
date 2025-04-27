#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

int create_socket();
void setup_address(struct sockaddr_in* address, int port);
int enable_socket_options(int socket_fd);
int bind_and_listen(int socket_fd, struct sockaddr_in* address);

#endif