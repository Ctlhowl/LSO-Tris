#define _XOPEN_SOURCE 700

#include <signal.h>
#include <stdatomic.h>  

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

static volatile sig_atomic_t shutdown_requested = 0;
static server_t server;

static pthread_t* threads[MAX_CLIENTS];
static int count = 0;

void* accept_clients(void* arg);
void handle_sig(int sig);

int main() {
    // Configurazione del signal handler
    struct sigaction sa = {
        .sa_handler = handle_sig,
        .sa_flags = 0
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Inizializzazione del server
    server_init(&server, DEFAULT_PORT);
    client_init(&server); 
    game_init(&server);

    if (!server_start(&server)) {
        return 1;
    }

    // Loop principale
    while (!shutdown_requested) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server.socket_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_sock < 0) {
            if (shutdown_requested) break;
            perror("accept failed");
            continue;
        }

        // Crea un thread per il client
        thread_args_t* args = malloc(sizeof(thread_args_t));
        args->client_sock = client_sock;
        args->server = &server;

        pthread_t thread;
        if (pthread_create(&thread, NULL, accept_clients, args) != 0) {
            perror("pthread_create failed");
            close(client_sock);
            free(args);
        } else {
            threads[count++] = &thread;
            pthread_detach(thread);
        }
    }

    // Cleanup sicuro (eseguito dal thread principale)
    game_cleanup(&server);
    client_cleanup(&server);
    server_close(&server);
    for(int i = 0; i < count; i++){
        pthread_cancel(*threads[i]);
    }
    return 0;
}

// Handler per SIGINT (Async-Signal-Safe)
void handle_sig(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        shutdown_requested = 1;  // Solo un flag atomico
    }
}

// Thread per client
void* accept_clients(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    int client_sock = args->client_sock;
    server_t* server = args->server;
    free(args);

    while (!shutdown_requested) {
        json_t* request = receive_json(client_sock);
        if (!request) break;

        const char* request_type = json_string_value(json_object_get(request, "request"));
        if (request_type && strcmp(request_type, DISCONNECT_MESSAGE) == 0) {
            json_decref(request);
            break;
        }

        handle_request(server, client_sock, request);
        json_decref(request);
    }

    // Cleanup del client
    const char* username = find_username_by_client(server, client_sock);
    if (username) {
        remove_games_by_username(server, username, client_sock);
    }
    
    client_remove(server, client_sock);
    close(client_sock);

    count--;
    pthread_exit(NULL);
}