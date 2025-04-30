#ifndef MESSAGES_H
#define MESSAGES_H

#include <jansson.h>
#include <stdbool.h>
#include <server.h>

bool send_broadcast(server_t* server, json_t* json_data, const int exclude_client);
bool send_to_player(server_t* server, json_t* json_data, const char* player);
bool send_json_message(json_t* json_data, int client_sock);
bool send_error(const int client_sock, const char* code_error, const char* description);

json_t* create_request(const char* request_type, const char* description, json_t* game_param);
json_t* create_response(const char* response_type, const char* description, json_t* game_param);
json_t* create_event(const char* event_type, size_t game_id, json_t* game_param);

json_t* receive_json(const int socket_fd);

#endif