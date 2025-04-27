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
    int socket_fd;
    struct sockaddr_in address;
    pthread_mutex_t clients_mutex;
    pthread_mutex_t games_mutex;
    bool running;
} server_t;

void server_init(server_t *config, int port);
int server_start(server_t *config);
void server_stop(server_t* server);
void server_cleanup(server_t *config);

#endif