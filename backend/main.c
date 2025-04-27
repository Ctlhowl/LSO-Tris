#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "server.h"
#include "client.h"
#include "network.h"
#include "game.h"
#include "messages.h"

server_t server;

void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    server_stop(&server);
    exit(0);
}

void* accept_clients(void* arg) {
    server_t* server = (server_t*)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    printf("Accepting connections on port %d\n", ntohs(server->address.sin_port));
    
    while(server->running) {
        int client_sock = accept(server->socket_fd, (struct sockaddr*)&client_addr, &addr_len);
        if(client_sock < 0) {
            if(server->running) {
                perror("accept failed");
            }
            continue;
        }
        
        // Ricevi il nome utente come primo messaggio
        char username[64];
        ssize_t bytes_received = recv(client_sock, username, sizeof(username)-1, 0);
        if(bytes_received <= 0) {
            close(client_sock);
            continue;
        }
        username[bytes_received] = '\0';
        
        // Verifica che l'username sia unico
        if(is_username_unique(username) != 0) {
            send_error(client_sock, "Username already in use");
            close(client_sock);
            continue;
        }
        
        // Crea e aggiungi il client
        client_t new_client;
        new_client.socket = client_sock;
        strncpy(new_client.username, username, sizeof(new_client.username));
        client_add(new_client);
        
        // Crea un thread per gestire il client
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, client_handler, &new_client) != 0) {
            perror("pthread_create failed");
            client_remove(client_sock);
            continue;
        }
        pthread_detach(thread_id);
        
        printf("New client connected: %s\n", username);
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    
    // Gestisci gli argomenti da riga di comando
    if(argc > 1) {
        port = atoi(argv[1]);
        if(port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number\n");
            return 1;
        }
    }
    
    // Configura la gestione dei segnali
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Inizializza il server
    server_init(&server, port);
    
    // Avvia il server
    if(server_start(&server) != 0) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }
    
    // Thread per accettare connessioni
    pthread_t accept_thread;
    if(pthread_create(&accept_thread, NULL, accept_clients, &server) != 0) {
        perror("pthread_create failed");
        server_stop(&server);
        return 1;
    }
    
    printf("Tic-Tac-Toe server running on port %d\n", port);
    printf("Press Ctrl+C to stop...\n");
    
    // Attendi il thread di accettazione (in pratica, il main thread resta in attesa)
    pthread_join(accept_thread, NULL);
    
    // Pulizia
    server_cleanup(&server);
    return 0;
}