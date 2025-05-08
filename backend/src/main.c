#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server.h"

#include "client.h"
#include "game.h"
#include "messages.h"
#include "routing.h"

typedef struct {
    int client_sock;
    server_t* server;
} thread_args_t;

server_t server;

void* accept_clients(void* arg);

int main() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Avvio server
    server_init(&server, DEFAULT_PORT);
    client_init(&server);
    game_init(&server);

    if(!server_start(&server)) {
        return 1;
    }
    
    while (server.running){
        int *client_sock = malloc(sizeof(int));
        if (!client_sock) continue;

        *client_sock = accept(server.socket_fd, (struct sockaddr*)&client_addr, &addr_len);
        if(*client_sock < 0) {
            printf("[Errore - main.main] accept fallita\n");
            free(client_sock);
            continue;
        }

        // Thread per accettare connessioni
        thread_args_t* args = malloc(sizeof(thread_args_t));
        args->client_sock = *client_sock;
        args->server = &server;
        free(client_sock);

        pthread_t accept_thread;
        if(pthread_create(&accept_thread, NULL, accept_clients, args) != 0) {
            printf("[Errore - main.main] Errore nella creazione del thread\n");
            close(args->client_sock);
            free(args);

            if (!server.running) break;
        }

        
        pthread_detach(accept_thread);
    }
    
    // Chiusura del server
    game_cleanup(&server);
    client_cleanup(&server);
    server_close(&server);
    return 0;
}

void* accept_clients(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;

    int client_sock = args->client_sock;
    server_t* server = args->server;
    free(args);

    printf("[Info - main.accept_clinet] Client accettato: %d\n", client_sock);

    bool running = true;
    while (running){
        json_t* request = receive_json(client_sock);

        if (!request) {
            running = false;
            break;
        }

        const char* request_type = json_string_value(json_object_get(request, "request"));
        if (request_type && strcmp(request_type, DISCONNECT_MESSAGE) == 0) {
            running = false;
            break;
        }
        
        handle_request(server, client_sock, request);
    }

    client_remove(server, client_sock);
    printf("[Info - main.accept_client] Client disconnected: %d\n", client_sock);
    return NULL;
}