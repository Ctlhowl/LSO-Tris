#include "server.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

//============ INTERFACCIA PUBBLICA ==================//
/**
 * Inizializza le variabili utili a gestire le informazioni del server
 */
void server_init(server_t *server, const int port) {
    // Inizializza i campi della struttura
    server->socket_fd = -1;
    server->running = false;
    
    // Inizializza i mutex
    pthread_mutex_init(&server->clients_mutex, NULL);
    pthread_mutex_init(&server->games_mutex, NULL);
    
    // Configura l'indirizzo del server
    memset(&server->address, 0, sizeof(server->address));
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(port);
}

/**
 * Crea una socket di tipo STREAM per il dominio TCP/IP, la imposta in modalità
 * SO_REUSEADDR in modo da consentire il riutilizzo degli indirizzi.
 */
bool server_start(server_t *server) {
    // Crea il socket
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        printf("[Errore - server.server_start] Creazione della socket fallita\n");
        return false;
    }
    
    // Imposta opzioni del socket
    int opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        printf("[Errore - server.server_start] setsockopt fallita\n");
        close(server->socket_fd);
        return false;
    }

    // Associa il socket all'indirizzo
    if (bind(server->socket_fd, (struct sockaddr*)&server->address, sizeof(server->address)) < 0) {
        printf("[Errore - server.server_start] bind fallita\n");
        close(server->socket_fd);
        return false;
    }
    
    // Mette il server in ascolto
    if (listen(server->socket_fd, MAX_CLIENTS) < 0) {
        printf("[Errore - server.server_start] listen fallita\n");
        close(server->socket_fd);
        return false;
    }
    
    server->running = true;
    printf("[Info - server.server_start] Server in ascolto sulla porta %d...\n", ntohs(server->address.sin_port));
    
    return true;
}


/**
 * Libera la memoria e imposta i campi della struttura ai valori di default prima di chiudere il server
 */
void server_close(server_t *server) {
    pthread_mutex_destroy(&server->clients_mutex);
    pthread_mutex_destroy(&server->games_mutex);
    
    if (server->running) {
        server->running = false;
    
        // Chiudi il socket
        if (server->socket_fd != -1) {
            close(server->socket_fd);
            server->socket_fd = -1;
        }
        
        printf("[Info - server.server_close] Server chiuso\n");
    }
}

