#ifndef MESSAGES_H
#define MESSAGES_H

#include <jansson.h>
#include <stdbool.h>
#include <server.h>

#include "game.h"

#define MAX_JSON_SIZE 1048576  // 1 MB

bool send_game_update(server_t* server, game_t* game, const char* username);
bool send_broadcast(server_t* server, json_t* json_data, const int exclude_client1, const int exclude_client2);
bool send_to_player(server_t* server, json_t* json_data, const char* player);
bool send_json_message(json_t* json_data, int client_sock);

json_t* create_request(const char* request_type, const char* description, json_t* game_param);
json_t* create_response(const char* response_type, bool status, const char* description, json_t* data);
json_t* create_broadcast(const char* event_type, json_t* game_param);

json_t* receive_json(const int socket_fd);

#endif