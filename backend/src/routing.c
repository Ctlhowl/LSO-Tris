#include "routing.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "server.h"
#include "client.h"
#include "game.h"
#include "messages.h"


//============ METODI PRIVATI ==================//

void handle_create_game(server_t* server, const int client_sock, const char* username){
    ssize_t game_id = create_game(server, username);

    if(game_id >= 0){
        json_t* response = create_response("game_created", "Game created", create_json(server, game_id));
        send_json_message(response, client_sock);

        // Notifica tutti i client della creazione di un nuovo gioco
        send_broadcast(server, create_event("new_game_available", game_id, create_json(server, game_id)), client_sock, -1);
        return;
    }

    send_error(client_sock, "507", "Internal server error");
}

void handle_join_request(server_t* server, const int client_sock, const char* username, const json_t* json_data){
    ssize_t game_id = json_integer_value(json_object_get(json_data, "game_id"));

    if (game_id >= 0) {
        // La logica di join_request potrebbe inviare una richiesta al creatore
        short result = request_join_game(server, game_id, username);
        switch(result){
            case 0:
                // Risposta alla richiesta del client
                json_t* response = create_response("join_requested", "Request successfully sended", create_json(server, game_id));
                send_json_message(response, client_sock);
                return;
            case -1:
                send_error(client_sock, "400", "Game id not exists");
                break;
            case -2:
                send_error(client_sock, "400", "Game isn't available");
                break;
            default:
        }
    }

    send_error(client_sock, "507", "Internal server error");
}

void handle_accept_join(server_t* server, const int client_sock, const json_t* json_data){
    size_t game_id = json_integer_value(json_object_get(json_data, "game_id"));
    const char* opponent = json_string_value(json_object_get(json_data, "player"));

    // Il creatore accetta la richiesta di join
    short result = accept_join_request(server, game_id, opponent);
    
    switch(result){
        case 0:
            // Risposta alla richiesta del client
            json_t* response = create_response("join_accepted", "", create_json(server, game_id));
            send_json_message(response, client_sock);

            // Notifica a entrambi i giocatori che la partita è iniziata
            send_game_update(server, find_game_by_id(game_id));

            printf("riga 70\n");
            int sock_client2 = find_client_by_username(server, opponent);
            // Notifica tutti i client che il game con id game_id non é piu disponibile
            send_broadcast(server, create_event("game_not_available", game_id, create_json(server, game_id)), client_sock, sock_client2);
            break;
        case -1:
            send_error(client_sock, "400", "Game id not exists");
            break;
        case -2:
            send_error(client_sock, "400", "Game isn't available");
            break;
        case -4:
            send_error(client_sock, "400", "The opponent is busy in another game");
            break;
        default:
            return;
    }
}

//============ INTERFACCIA PUBBLICA ==================//
void handle_route(server_t* server, const int client_sock, const char* username, const json_t* json_data){
    json_t *json_method = json_object_get(json_data, "method");
    
    if(!json_is_string(json_method)){
        send_error(client_sock, "405", "Invalid Method");
        return;
    }

    const char* method = json_string_value(json_method);
    if(strcmp(method, "create_game") == 0){
        handle_create_game(server, client_sock, username);
        return;
    }

    if(strcmp(method, "join_request") == 0){
        handle_join_request(server, client_sock, username, json_data);
        return;
    }

    if(strcmp(method, "accept_join") == 0){
        handle_accept_join(server, client_sock, json_data);
        return;
    }

    if (strcmp(method, "reject_join") == 0){
        
        return;
    }

    if (strcmp(method, "list_games") == 0){
        
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
        send_error(client_sock, "405", "Invalid Method");
        return;
    }
}


/**
 * Gestione richiesta login
 */
bool handle_login(server_t* server, const int client_sock, const json_t* json_method, const json_t* json_user) {
    const char* method = json_string_value(json_method);
    
    if(!json_is_string(json_method) || !json_is_string(json_user)){
        send_error(client_sock, "400", "Method or Username should be string");
        return false;
    }

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

    send_error(client_sock, "400", "Invalid Method");
    return false;
}