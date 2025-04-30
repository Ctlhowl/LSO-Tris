#ifndef ROUTING_H
#define ROUTING_H

#include <jansson.h>
#include <stdbool.h>
#include <server.h>

void handle_route(server_t* server, const int client_sock, const char* username, const json_t* json_data);
bool handle_login(server_t* server, const int client_sock, const json_t* json_method, const json_t* json_user);

#endif