#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Definizione delle variabili globali
client_list_t* connected_clients = NULL;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void client_init_globals() {
    connected_clients = (client_list_t*)malloc(sizeof(client_list_t));
    if (!connected_clients) {
        perror("Failed to allocate memory for connected clients list");
        exit(EXIT_FAILURE);
    }
    connected_clients->head = NULL;
    connected_clients->count = 0;
}

void client_add(client_t client) {
    pthread_mutex_lock(&clients_mutex);
    
    // Crea un nuovo nodo
    client_node_t* new_node = (client_node_t*)malloc(sizeof(client_node_t));
    if (!new_node) {
        perror("Failed to allocate memory for new client node");
        pthread_mutex_unlock(&clients_mutex);
        return;
    }
    
    new_node->client = client;
    new_node->next = connected_clients->head;
    connected_clients->head = new_node;
    connected_clients->count++;
    
    printf("New client connected: %s (socket %d)\n", client.username, client.socket);
    
    pthread_mutex_unlock(&clients_mutex);
}

void client_remove(int socket) {
    pthread_mutex_lock(&clients_mutex);
    
    client_node_t** pp = &connected_clients->head;
    client_node_t* current = connected_clients->head;
    
    while (current) {
        if (current->client.socket == socket) {
            *pp = current->next;
            printf("Client disconnected: %s (socket %d)\n", 
                   current->client.username, current->client.socket);
            close(current->client.socket);
            free(current);
            connected_clients->count--;
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
        pp = &current->next;
        current = current->next;
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

int find_client_by_username(const char* username) {
    if (!username) return -1;
    
    pthread_mutex_lock(&clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (strcmp(current->client.username, username) == 0) {
            int socket = current->client.socket;
            pthread_mutex_unlock(&clients_mutex);
            return socket;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&clients_mutex);
    return -1; // Not found
}

bool is_username_unique(const char* username) {
    if (!username) return false;
    
    pthread_mutex_lock(&clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (strcmp(current->client.username, username) == 0) {
            pthread_mutex_unlock(&clients_mutex);
            return false;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&clients_mutex);
    return true;
}

void* client_handler(void* arg) {
    client_t* client = (client_t*)arg;
    char buffer[1024];
    ssize_t bytes_received;
    
    printf("Handler started for client %s\n", client->username);
    
    while ((bytes_received = recv(client->socket, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received from %s: %s\n", client->username, buffer);
        
        // Echo response (sostituire con la logica del gioco)
        send(client->socket, buffer, bytes_received, 0);
    }
    
    if (bytes_received == 0) {
        printf("Client %s disconnected\n", client->username);
    } else {
        perror("recv failed");
    }
    
    client_remove(client->socket);
    return NULL;
}