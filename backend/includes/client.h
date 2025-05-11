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

/**
 * Inizializza le strutture utili per gestire i client connessi
 */
void client_init(server_t* server);

/**
 * Libera la memoria allocata per gestire i client connessi 
 */
void client_cleanup(server_t* server);

/**
 * Aggiunge un client alla lista di client connessi al server
 * Ritorna true se il client è stato aggiunto correttamente, false altrimenti
 */
bool client_add(server_t* server, const client_t* client);

/**
 * Rimuove il client dalla lista di client connessi.
 * Ritorna verso se il client è stato rimosso, false altrimenti
 */
bool client_remove(server_t* server, const ssize_t sock);

/**
 * Cerca la in numero di socket del client in base all'username (univoco) del player. 
 * Ritorna il numero della socket a cui è connesso il client se il player esiste, -1 altrimenti 
 */
ssize_t find_client_by_username(server_t* server, const char* username);

/**
 * Cerca l'username (univoco) del player in base al numero di socket. 
 * Ritorna il numero l'username del player se esiste, NULL altrimenti 
 */
const char* find_username_by_client(server_t* server, const ssize_t sock);

/**
 * Verifica se l'username è univoco. 
 * Ritorna vero se l'username è univoco, false altrimenti
 */
bool is_username_unique(server_t* server, const char* username);

#endif