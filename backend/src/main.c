#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server.h"

#include "client.h"
#include "game.h"
#include "messages.h"
#include "routing.h"

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
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }
    
    while (server.running){
        int *client_sock = malloc(sizeof(int));
        if (!client_sock) continue;

        *client_sock = accept(server.socket_fd, (struct sockaddr*)&client_addr, &addr_len);
        if(*client_sock < 0) {
            perror("accept failed");
            free(client_sock);
            continue;
        }

        // Thread per accettare connessioni
        pthread_t accept_thread;
        if(pthread_create(&accept_thread, NULL, accept_clients, client_sock) != 0) {
            perror("pthread_create failed");
            close(*client_sock);
            free(client_sock);

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
    int client_sock = *(int*)arg;
    free(arg);

    printf("New connection on socket %d\n", client_sock);

    // Invio richiesta login
    json_t* request = create_request("login", "Please send your username", NULL);
    if (!request) {
        perror("create request failed");
        close(client_sock);
        return NULL;
    }

    if (!send_json_message(request, client_sock)) {
        perror("send message failed");
        close(client_sock);
        return NULL;
    }
    
    // Gestione autenticazione
    while (true){
        json_t* response = receive_json(client_sock);
        if (response) {
            // Valuta risposta
            json_t *json_method = json_object_get(response, "method");
            json_t *json_user = json_object_get(response, "username");
            
            if(handle_login(&server, client_sock, json_method, json_user)){
                
                // Gestione routing
                while (true){
                    json_t* response = receive_json(client_sock);        
                    if (!response) {
                        client_remove(&server, client_sock);
                        return NULL;
                    }

                    const char* username = json_string_value(json_user);
                    handle_route(&server, client_sock, username, response);
                }
            }
        } 
    }
}