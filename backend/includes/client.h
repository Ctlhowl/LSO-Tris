#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <stdbool.h>
#include <server.h>

typedef struct {
    ssize_t socket;
    char username[64];
} client_t;

typedef struct client_node {
    client_t client;
    struct client_node* next;
} client_node_t;

typedef struct {
    client_node_t* head;
    size_t count;
} client_list_t;

extern client_list_t* connected_clients;

void client_init(server_t* server);
void client_cleanup(server_t* server);

bool client_add(server_t* server, const client_t* client);
bool client_remove(server_t* server, const ssize_t sock);

ssize_t find_client_by_username(server_t* server, const char* username);
const char* find_username_by_client(server_t* server, const ssize_t sock);

bool is_username_unique(server_t* server, const char* username);

#endif