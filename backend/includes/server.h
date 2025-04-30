#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

#define MAX_CLIENTS 100
#define MAX_GAMES 50
#define DEFAULT_PORT 8080

typedef struct {
    ssize_t socket_fd;
    bool running;
    struct sockaddr_in address;
    pthread_mutex_t clients_mutex;
    pthread_mutex_t games_mutex;
} server_t;

void server_init(server_t *server, int port);
bool server_start(server_t *server);
void server_close(server_t *server);

#endif