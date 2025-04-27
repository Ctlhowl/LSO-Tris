#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>

void server_init(server_t *config, int port) {
    // Inizializza la struttura del server
    config->socket_fd = -1;
    config->running = false;
    
    // Inizializza i mutex
    pthread_mutex_init(&config->clients_mutex, NULL);
    pthread_mutex_init(&config->games_mutex, NULL);
    
    // Configura l'indirizzo del server
    memset(&config->address, 0, sizeof(config->address));
    config->address.sin_family = AF_INET;
    config->address.sin_addr.s_addr = INADDR_ANY;
    config->address.sin_port = htons(port);
}

int server_start(server_t *config) {
    // Crea il socket
    config->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (config->socket_fd < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    // Imposta opzioni del socket
    int opt = 1;
    if (setsockopt(config->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(config->socket_fd);
        return -1;
    }
    
    // Associa il socket all'indirizzo
    if (bind(config->socket_fd, (struct sockaddr*)&config->address, sizeof(config->address)) < 0) {
        perror("bind failed");
        close(config->socket_fd);
        return -1;
    }
    
    // Metti il server in ascolto
    if (listen(config->socket_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(config->socket_fd);
        return -1;
    }
    
    config->running = true;
    printf("server_t started on port %d\n", ntohs(config->address.sin_port));
    
    return 0;
}

void server_stop(server_t* server) {
    if (!server->running) return;
    
    server->running = false;
    
    // Chiudi il socket
    if (server->socket_fd != -1) {
        close(server->socket_fd);
        server->socket_fd = -1;
    }
    
    printf("server_t stopped\n");
}

void server_cleanup(server_t *config) {
    // Distruggi i mutex
    pthread_mutex_destroy(&config->clients_mutex);
    pthread_mutex_destroy(&config->games_mutex);
    
    // Se il server è ancora in esecuzione, fermalo
    if (config->running) {
        server_stop(config);
    }
}