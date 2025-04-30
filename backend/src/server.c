#include "server.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

//============ INTERFACCIA PUBBLICA ==================//
/**
 * Inizializza le variabili della struttura server_t e i mutex
 */
void server_init(server_t *server, const int port) {
    // Inizializza la struttura del server
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
 * Per i socket AF_INET, questo significa che un socket può effettuare il binding,
 * tranne quando è presente un socket in ascolto attivo associato all'indirizzo.
 * Quando il socket in ascolto è associato a INADDR_ANY con una porta specifica,
 * non è possibile associare a questa porta alcun indirizzo locale.
 */
bool server_start(server_t *server) {
    // Crea il socket
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        perror("socket creation failed");
        return false;
    }
    
    // Imposta opzioni del socket
    int opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server->socket_fd);
        return false;
    }

    /*
    struct timeval tv = {30, 0}; // 30 second timeout
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
        perror("setsockopt failed");
        close(server->socket_fd);
        return false;
    }
    */

    // Associa il socket all'indirizzo
    if (bind(server->socket_fd, (struct sockaddr*)&server->address, sizeof(server->address)) < 0) {
        perror("bind failed");
        close(server->socket_fd);
        return false;
    }
    
    // Mette il server in ascolto
    if (listen(server->socket_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(server->socket_fd);
        return false;
    }
    
    server->running = true;
    printf("Server listening on port %d...\n", ntohs(server->address.sin_port));

    return true;
}


/**
 * Si occupa resettare i valori della struttura server_t e di deallocare i mutex
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
        
        printf("Server_t stopped\n");
    }
}

