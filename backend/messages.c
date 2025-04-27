#include <jansson.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include "client.h"
#include "messages.h"

void broadcast_json(json_t* msg, int exclude_fd) {
    if (!msg) return;

    char* msg_str = json_dumps(msg, JSON_COMPACT);
    if (!msg_str) return;

    pthread_mutex_lock(&clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (current->client.socket != exclude_fd) {
            if (send(current->client.socket, msg_str, strlen(msg_str), 0) < 0) {
                perror("broadcast send failed");
            }
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&clients_mutex);
    free(msg_str);
}

void send_to_player(const char* username, json_t* message) {
    if (!username || !message) return;

    char* msg_str = json_dumps(message, JSON_COMPACT);
    if (!msg_str) return;

    pthread_mutex_lock(&clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (strcmp(current->client.username, username) == 0) {
            if (send(current->client.socket, msg_str, strlen(msg_str), 0) < 0) {
                perror("send_to_player failed");
            }
            break;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&clients_mutex);
    free(msg_str);
}

json_t* create_message(const char* event_type) {
    json_t* msg = json_object();
    if (!msg) return NULL;

    if (json_object_set_new(msg, "event", json_string(event_type)) != 0) {
        json_decref(msg);
        return NULL;
    }
    
    // Aggiunge timestamp
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    json_object_set_new(msg, "timestamp", json_string(timestamp));
    
    return msg;
}

void send_error(int sock, const char* error_msg) {
    if (!error_msg || sock < 0) return;

    json_t* error = create_message("error");
    if (!error) return;

    json_object_set_new(error, "message", json_string(error_msg));
    
    char* error_str = json_dumps(error, JSON_COMPACT);
    if (error_str) {
        send(sock, error_str, strlen(error_str), 0);
        free(error_str);
    }
    
    json_decref(error);
}