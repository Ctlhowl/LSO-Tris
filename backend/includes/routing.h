#ifndef ROUTING_H
#define ROUTING_H

#include <jansson.h>
#include <stdbool.h>
#include <server.h>

/** 
 * Gestisce le varie richieste inviate dal client 
*/
void handle_request(server_t* server, const int client_sock, const json_t* json_request);

#endif