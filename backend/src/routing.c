#include "routing.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "server.h"
#include "client.h"
#include "game.h"
#include "messages.h"

//============ INTERFACCIA PUBBLICA ==================//
void handle_route(server_t* server, const int client_sock, const char* username, const json_t* json_data){
    json_t *json_method = json_object_get(json_data, "method");
    
    if(!json_is_string(json_method)){
        send_error(client_sock, "400", "Invalid Method");
    }

    const char* method = json_string_value(json_method);
    if(strcmp(method, "create_game") == 0){
        ssize_t game_id = create_game(server, username);

        json_t* json_game = create_json(server, game_id);
        json_t* response = create_response("game_created", "Game created", json_game);
        if (!response) {
            send_error(client_sock, "507", "Internal server error");
            return;
        }

        send_json_message(response, client_sock);
        return;
    }

    if(strcmp(method, "join_request") == 0){

        return;
    }

    if(strcmp(method, "accept_join") == 0){

        return;
    }

    if (strcmp(method, "reject_join") == 0){
        
        return;
    }

    if (strcmp(method, "list_available_games") == 0){
        
        return;
    }

    if (strcmp(method, "list_user_games") == 0){
        
        return;
    }

    if (strcmp(method, "game_move") == 0){
        
        return;
    }

    if (strcmp(method, "quit") == 0){
        
        return;
    }

    if (strcmp(method, "rematch") == 0){
        
        return;
    }else{
        send_error(client_sock, "405", "Method Not Allowed");
    }

}


/**
 * Gestione richiesta login
 */
bool handle_login(server_t* server, const int client_sock, const json_t* json_method, const json_t* json_user) {
    const char* method = json_string_value(json_method);

    if (method && strcmp(method, "login") == 0) {
        const char* username = json_string_value(json_user);
        
        // Controllo sull'username
        if (!username) {
            send_error(client_sock, "400", "Username required");
            return false;
        }

        size_t username_len = strlen(username);
        if (username_len == 0 || username_len >= 64) {
            send_error(client_sock, "400", "Username must be 1-63 characters");
            return false;
        }

        // Verifica unicità del nome
        if (!is_username_unique(server, username)) {
            send_error(client_sock, "409", "Username already in use");
            return false;
        }

        // Crea un nuovo client
        client_t* new_client = (client_t*)malloc(sizeof(client_t));
        if (!new_client) {
            send_error(client_sock, "507", "Internal server error");
            return false;
        }

        new_client->socket = client_sock;
        strncpy(new_client->username, username, strlen(username));

        // Aggiunge il client alla lista di client connessi
        if (!client_add(server, new_client)) {
            send_error(client_sock, "507", "Internal server error");
            return false;
        }


        json_t* response = create_response("login_success", "Welcome to the game", NULL);
        if (!response) {
            send_error(client_sock, "507", "Internal server error");
            return false;
        }

        send_json_message(response, client_sock);
        return true;
    }

    return false;
}