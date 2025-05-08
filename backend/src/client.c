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
            printf("[Errore - client.client_init] Impossibile allocare memoria per la lista dei client connessi\n");
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
 * Aggiunge un client alla lista di client connessi al server
 * Ritorna true se il client è stato aggiunto correttamente, false altrimenti
 */
bool client_add(server_t* server, const client_t* client) {
    pthread_mutex_lock(&server->clients_mutex);
    
    // Controllo disponibilità slot client 
    if(connected_clients->count >= MAX_CLIENTS){
        pthread_mutex_unlock(&server->clients_mutex);
        printf("[Errore - client.client_add] Impossibile aggiungere client, il server è pieno\n");
        return false;
    }

    // Alloca memoria per il nuovo client
    client_node_t* new_node = (client_node_t*)malloc(sizeof(client_node_t));
    if (!new_node) {
        printf("[Errore - client.client_add] Impossibile allocare memoria un nuovo client\n");

        pthread_mutex_unlock(&server->clients_mutex);
        return false;
    }

    // Aggiungo il client alla struttura
    new_node->client = *client;
    new_node->next = connected_clients->head;
    connected_clients->head = new_node;
    connected_clients->count++;
    
    printf("[Info - client.client_add] Nuovo client connesso: %s (socket %ld)\n", client->username, client->socket);
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
            
            printf("[Info - client.client_remove] Client disconnesso: %s (socket %ld)\n", current->client.username, current->client.socket);
            close(current->client.socket);
            memset(&current->client, 0, sizeof(client_t));
            
            free(current);

            connected_clients->count--;

            pthread_mutex_unlock(&server->clients_mutex);
            return true;
        }
        pp = &current->next;
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);

    printf("[Errore - client.client_remove] Il client che si sta cercando di rimuovere non esiste\n");
    return false;
}

/**
 * Cerca la in numero di socket del client in base all'username (univoco) del player. 
 * Ritorna il numero della socket a cui è connesso il client se il player esiste, -1 altrimenti 
 */
ssize_t find_client_by_username(server_t* server, const char* username) {
    if (strlen(username) == 0){
        printf("[Errore - client.find_client_by_username] Il nome del giocatore non valido\n");
        return -1;
    }

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
    printf("[Errore - client.find_client_by_username] Il client associato giocatore %s non esiste\n", username);
    return -1;
}

/**
 * Cerca l'username (univoco) del player in base al numero di socket. 
 * Ritorna il numero l'username del player se esiste, NULL altrimenti 
 */
const char* find_username_by_client(server_t* server, const ssize_t sock) {
    if (sock == -1){
        printf("[Errore - client.find_username_by_client] Client non valido\n");
        return NULL;
    }
    
    pthread_mutex_lock(&server->clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (current->client.socket == sock) {
            const char* username = current->client.username;

            pthread_mutex_unlock(&server->clients_mutex);
            return username;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
    printf("[Errore - client.find_username_by_client] Il client %ld non esiste\n", sock);
    return NULL;
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