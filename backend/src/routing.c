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
        json_t* response = create_response("game_created", "Partita creata con successo", create_json(server, game_id));
        send_json_message(response, client_sock);

        // Notifica tutti i client della creazione di un nuovo gioco
        send_broadcast(server, create_event("new_game_available", game_id, create_json(server, game_id)), client_sock, -1);
        return;
    }

    json_t* error = create_response("error","Errore interno al server",NULL);
    send_json_message(error, client_sock);
}

void handle_join_request(server_t* server, const int client_sock, const char* username, const json_t* json_data){
    json_t* error = json_object();
    ssize_t game_id = json_integer_value(json_object_get(json_data, "game_id"));

    if (game_id >= 0) {
        short result = request_join_game(server, game_id, username);
        switch(result){
            case 0:
                // Risposta alla richiesta del client
                send_json_message(create_response("join_requested", "Richiesta inviata correttamente", create_json(server, game_id)), client_sock);
                return;
            case -1:
                send_json_message(create_response("error", "Id partita inesistente", NULL), client_sock);
                break;
            case -2:
                send_json_message(create_response("error", "La parita non esiste più", NULL), client_sock);
                break;
            default:
        }
    }

    send_json_message(create_response("error", "Errore Server", NULL), client_sock);
}

void handle_accept_join(server_t* server, const int client_sock, const json_t* json_data){
    json_t* error = json_object();
    size_t game_id = json_integer_value(json_object_get(json_data, "game_id"));
    const char* opponent = json_string_value(json_object_get(json_data, "player"));

    // Il creatore accetta la richiesta di join
    short result = accept_join_request(server, game_id, opponent);
    
    switch(result){
        case 0:
            // Risposta alla richiesta del client
            json_t* response = create_response("join_accepted", "Richiesta accettata con successo", create_json(server, game_id));
            send_json_message(response, client_sock);

            // Notifica a entrambi i giocatori che la partita è iniziata
            send_game_update(server, find_game_by_id(server,game_id));

            int sock_client2 = find_client_by_username(server, opponent);
            // Notifica tutti i client che il game con id game_id non é piu disponibile
            send_broadcast(server, create_event("game_not_available", game_id, create_json(server, game_id)), client_sock, sock_client2);
            break;
        case -1:
            error = create_response("error","La partita non esiste",NULL);
            send_json_message(error, client_sock);
            break;
        case -2:
            error = create_response("error","Partita non disponibile",NULL);
            send_json_message(error, client_sock);
            break;
        case -4:
            error = create_response("error","Avversario impegnato in un altra partita",NULL);
            send_json_message(error, client_sock);
            break;
        default:
            error = create_response("error","Errore interno al server",NULL);
            send_json_message(error, client_sock);
            return;
    }
}

void handle_reject_join(const int client_sock, const json_t* json_data){
    size_t game_id = json_integer_value(json_object_get(json_data, "game_id"));
    const char* opponent = json_string_value(json_object_get(json_data, "player"));

    json_t* result = reject_join_request(game_id,opponent);

    if(result){
        json_t* response = create_response("join_rejected", "Richiesta rifiutata con successo", result);
        send_json_message(response, client_sock);
    }

    json_t* error = create_response("error","Impossibile rifiutare la richiesta",NULL);
    send_json_message(error, client_sock);
}

void handle_game_move(server_t* server, const int client_sock, const char* username, const json_t* json_data){
    json_t* error = json_object();
    size_t game_id = json_integer_value(json_object_get(json_data, "game_id"));
    short x = json_integer_value(json_object_get(json_data, "x"));
    short y = json_integer_value(json_object_get(json_data, "y"));

    game_t* game = find_game_by_id(server,game_id);

    if(!game){
        json_t* error = create_response("error","Partita non trovata",NULL);
        send_json_message(error, client_sock);
        return;
    }

    short result = make_move(server, game, username, x, y);

    switch(result){
        case 0 :
            send_game_update(server,game);
            break;
        case -1:
            error = create_response("error","La partita non è in gioco",NULL);
            send_json_message(error,client_sock);
            break;
        case -2:
            error = create_response("error","Attendi che sia il tuo turno per effettuare la mossa",NULL);
            send_json_message(error,client_sock);
            break;
        default:
            error = create_response("error","Errore interno al server",NULL);
            send_json_message(error, client_sock);
            return;
    }

}

void handle_list_games(server_t* server, const int client_sock, const char* username){
    json_t* games = list_games(server,username);

    if(games){
        json_t* response = create_response("list_all_games", "List of all the games in the system", games);
        send_json_message(response,client_sock);
        return;
    }

    json_t* error = create_response("error","Errore nel recupero delle partite",NULL);
    send_json_message(error, client_sock);
}

//============ INTERFACCIA PUBBLICA ==================//
void handle_route(server_t* server, const int client_sock, const char* username, const json_t* json_data){
    json_t *json_method = json_object_get(json_data, "method");
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
        handle_reject_join(client_sock, json_data);
        return;
    }

    if (strcmp(method, "list_games") == 0){
        handle_list_games(server, client_sock, username);
        return;
    }

    if (strcmp(method, "game_move") == 0){
        handle_game_move(server,client_sock,username,json_data);
        return;
    }

    if (strcmp(method, "quit") == 0){
        
        return;
    }

    if (strcmp(method, "rematch") == 0){
        
        return;
    }else{
        json_t* error = create_response("error","Richiesta non valida",NULL);
        send_json_message(error, client_sock);
        return;
    }
}


/**
 * Gestione richiesta login
 */
bool handle_login(server_t* server, const int client_sock, const json_t* json_method, const json_t* json_user) {
    const char* method = json_string_value(json_method);

    if (method && strcmp(method, "login") == 0) {
        const char* username = json_string_value(json_user);

        // Verifica unicità del nome
        if (!is_username_unique(server, username)) {
            send_json_message(create_response("error", "Username già in uso, riprova", NULL), client_sock);
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
