#ifndef MESSAGES_H
#define MESSAGES_H

#include <jansson.h>

// Funzioni per la gestione dei messaggi
void broadcast_json(json_t* msg, int exclude_fd);
void send_to_player(const char* player, json_t* message);
json_t* create_message(const char* event_type);
void send_error(int sock, const char* error_msg);
#endif