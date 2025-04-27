#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    int socket;
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
extern pthread_mutex_t clients_mutex;

void client_add(client_t client);
void client_remove(int sock);
int find_client_by_username(const char* username);
bool is_username_unique(const char* username);
void* client_handler(void* arg);

#endif