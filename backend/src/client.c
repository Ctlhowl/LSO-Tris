#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

client_list_t* connected_clients = NULL;

//============ INTERFACCIA PUBBLICA ==================//
/**
 * Inizializza le strutture utili per gestire i client connessi
 */
void client_init(server_t* server) {
    pthread_mutex_lock(&server->clients_mutex);

    if (connected_clients == NULL) {
        connected_clients = (client_list_t*)malloc(sizeof(client_list_t));
        if (!connected_clients) {
            perror("Failed to allocate memory for connected clients list");
            exit(EXIT_FAILURE);
        }
        
        connected_clients->head = NULL;
        connected_clients->count = 0;
    }

    pthread_mutex_unlock(&server->clients_mutex);
}

/**
 * Libera la memoria allocata per gestire i client connessi 
 */
void client_cleanup(server_t* server) {
    pthread_mutex_lock(&server->clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        client_node_t* next = current->next;
        free(current->client.username);
        free(current);
        current = next;
    }
    
    free(connected_clients);
    connected_clients = NULL;
    
    pthread_mutex_unlock(&server->clients_mutex);
}

/**
 * Aggiunge un client alla lista di client connessi al server-> 
 * Ritorna true se il client è stato aggiunto correttamente, false altrimenti
 */
bool client_add(server_t* server, const client_t* client) {
    pthread_mutex_lock(&server->clients_mutex);
    
    // Controllo disponibilità slot client 
    if(connected_clients->count >= MAX_CLIENTS){
        pthread_mutex_unlock(&server->clients_mutex);
        return false;
    }

    // Alloca memoria per il nuovo client
    client_node_t* new_node = (client_node_t*)malloc(sizeof(client_node_t));
    if (!new_node) {
        perror("Failed to allocate memory for new client node");

        pthread_mutex_unlock(&server->clients_mutex);
        return false;
    }
    
    // Aggiungo il client alla struttura
    new_node->client = *client;
    new_node->next = connected_clients->head;
    connected_clients->head = new_node;
    connected_clients->count++;
    
    printf("New client connected: %s (socket %d)\n", client->username, client->socket);
    
    pthread_mutex_unlock(&server->clients_mutex);
    return true;
}

/**
 * Rimuove il client dalla lista di client connessi.
 * Ritorna verso se il client è stato rimosso, false altrimenti
 */
bool client_remove(server_t* server, const ssize_t socket) {
    pthread_mutex_lock(&server->clients_mutex);
    
    client_node_t** pp = &connected_clients->head;
    client_node_t* current = connected_clients->head;
    
    while (current) {
        if (current->client.socket == socket) {
            *pp = current->next;
            printf("Client disconnected: %s (socket %d)\n", current->client.username, current->client.socket);
                        
            close(current->client.socket);
            free(current);

            connected_clients->count--;

            pthread_mutex_unlock(&server->clients_mutex);
            return true;
        }
        pp = &current->next;
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
    return false;
}

/**
 * Cerca la in numero di socket del client in base all'username (univoco) del player. 
 * Ritorna il numero del fd della socket a cui è connesso il client se il player esiste, -1 altrimenti 
 */
int find_client_by_username(server_t* server, const char* username) {
    if (!username) return -1;

    pthread_mutex_lock(&server->clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (strcmp(current->client.username, username) == 0) {
            int socket = current->client.socket;

            pthread_mutex_unlock(&server->clients_mutex);
            return socket;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
    return -1;
}


/**
 * Verifica se l'username è univoco. 
 * Ritorna vero se l'username è univoco, false altrimenti
 */
bool is_username_unique(server_t* server, const char* username) {
    if (!username) return false;

    pthread_mutex_lock(&server->clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (strcmp(current->client.username, username) == 0) {

            pthread_mutex_unlock(&server->clients_mutex);
            return false;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
    return true;
}