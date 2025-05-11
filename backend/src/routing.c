#include "routing.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "server.h"
#include "client.h"
#include "game.h"
#include "messages.h"


//============ METODI PRIVATI ==================//
/**
 * Gestione richiesta login. Il metodo verifica che l'username sia univoco rispetto alla lista dei giocatori presenti nel server.
 * Se il nome è univoco allora il metodo invia la risposta la client di login con successo, altrimenti lo notifica dell'errore
 */
void handle_login(server_t* server, const int client_sock, const json_t* data) {
    const char* username = json_string_value(json_object_get(data, "username"));
    json_t* response;

    // Verifica unicità del nome
    if (!is_username_unique(server, username)) {
        response = create_response("login", false, "Username già in uso, riprova", NULL);
        send_json_message(response, client_sock);
        json_decref(response);
        return;
    }

    // Crea un nuovo client
    client_t* new_client = (client_t*)malloc(sizeof(client_t));
    if (!new_client) {
        response = create_response("login", false, "Errore Server", NULL);
        send_json_message(response, client_sock);
        json_decref(response);
        return;
    }
    
    memset(new_client, 0, sizeof(client_t));
    new_client->socket = client_sock;
    strncpy(new_client->username, username, sizeof(new_client->username) - 1);
    new_client->username[sizeof(new_client->username) - 1] = '\0';

    // Aggiunge il client alla lista di client connessi
    if (!client_add(server, new_client)) {
        free(new_client);

        response = create_response("login", false, "Errore Server", NULL);
        send_json_message(response, client_sock);
        json_decref(response);
        return;
    }
    free(new_client);
    
    response = create_response("login", true, "Benvenuto nel gioco", NULL);
    send_json_message(response, client_sock);
    json_decref(response);
}

/**
 * Gestisce la creazione di un nuovo gioco.
 * Crea la partita e invia al client le informazioni relative ad essa, altrimenti lo notifica dell'errore
 */
void handle_create_game(server_t* server, const int client_sock){
    const char* username = find_username_by_client(server, client_sock);
    ssize_t game_id = create_game(server, username);

    json_t* response;

    if(game_id >= 0){
        response = create_response("create_game", true, "Partita creata con successo", create_json(server, game_id, false));
        send_json_message(response, client_sock);
        json_decref(response);

        // Notifica tutti i client della creazione di un nuovo gioco
        send_broadcast(server, "new_game_available", create_json(server, game_id, false), client_sock, -1);
        return;
    }

    response = create_response("create_game", false, "Errore interno al server", NULL);
    send_json_message(response, client_sock);
    json_decref(response);
}

/**
 * Ricerca ed invia al client tutte le partite presenti tranne quelle create da se stesso.
 */
void handle_list_games(server_t* server, const int client_sock){
    const char* username = find_username_by_client(server, client_sock);
    json_t* games = list_games(server, username);
    json_t* response;

    if(games){
        response = create_response("list_games", true, "Lista delle partite disponibili", games);
        send_json_message(response, client_sock);
        json_decref(response);
        return;
    }

    response = create_response("list_games", false, "Errore nel recupero delle partite", NULL);
    send_json_message(response, client_sock);
    json_decref(response);
}

/**
 * Gestisce la richiesta per unirsi.
 * Viene creata e inviata la richiesta di unione al creatore della partita e notifica il richiedente se:
 * 0 - La richiesta è stata inviata correttamente
 * 1 - Id della partita non esiste
 * 2 - La partita è terminata
 * 3 - La partita è gia avviata
 * 4 - Errore generico lato server
 */
void handle_join_request(server_t* server, const int client_sock, const json_t* data){
    const char* username = find_username_by_client(server, client_sock);
    ssize_t game_id = json_integer_value(json_object_get(data, "game_id"));

    if (game_id >= 0) {
        short result = request_join_game(server, game_id, username);

        json_t* response;
        switch(result){
            case 0:
                response = create_response("join_request", true, "Richiesta inviata correttamente", NULL);
                send_json_message(response, client_sock);
                break;;
            case -1:
                response = create_response("join_request", false, "Id partita inesistente", NULL);
                send_json_message(response, client_sock);
                break;
            case -2:
                response = create_response("join_request", false, "La partita è terminata", NULL);
                send_json_message(response, client_sock);
                break;
            case -3:
                response = create_response("join_request", false, "La partita è già avviata", NULL);
                send_json_message(response, client_sock);
                break;
            default:
                response = create_response("join_request", false, "Errore Server", NULL);
                send_json_message(response, client_sock);
                break;
        }

        json_decref(response);
    }
}

/**
 * Gestisce se la mossa effettuata è valida rispetto al turno e la cella.
 */
void handle_game_move(server_t* server, const int client_sock, const json_t* data){
    json_t* response;

    size_t game_id = json_integer_value(json_object_get(data, "game_id"));
    game_t* game = find_game_by_id(server, game_id);
    
    if(!game){
        response = create_response("game_move", false, "Partita non trovata", NULL);
        send_json_message(response, client_sock);
        json_decref(response);
        return;
    }

    short x = json_integer_value(json_object_get(data, "x"));
    short y = json_integer_value(json_object_get(data, "y"));

    const char* username = find_username_by_client(server, client_sock);
    short result = make_move(server, game, username, x, y);

    switch(result){
        case 0 :
            send_game_update(server, game, username);
            return;
        case -1:
            response = create_response("game_move", false, "La partita non è in gioco", NULL);
            send_json_message(response, client_sock);
            break;
        case -2:
            response = create_response("game_move", false, "Attendi che sia il tuo turno per effettuare la mossa", NULL);
            send_json_message(response, client_sock);
            break;
        case -3:
            response = create_response("game_move", false, "Cella già occupata", NULL);
            send_json_message(response, client_sock);
            break;
        default:
            response = create_response("game_move", false, "Errore interno al server", NULL);
            send_json_message(response, client_sock);
            break;
    }

    json_decref(response);
}

/**
 * Gestione caso in cui il creatore della partita accetta la richiesta di join da parte di client_sock.
 */
void handle_accept_join(server_t* server, const int client_sock, const json_t* data){
    json_t* response;

    size_t game_id = json_integer_value(json_object_get(data, "game_id"));
    const char* opponent = json_string_value(json_object_get(data, "player2"));

    short result = accept_join_request(server, game_id, opponent);
    json_t* game_json = create_json(server, game_id, false);

    switch(result){
        case 0:
            // Notifica il creatore che la partita sta stata accettata con successo e può essere avviata
            response = create_response("accept_join", true, "Richiesta accettata con successo", game_json);
            json_t* request = create_request("game_started", "La partita sta per cominciare", json_deep_copy(game_json));
            
            send_json_message(response, client_sock);
            send_json_message(request, client_sock);

            // Notifica tutti i client che il game con id game_id non é piu disponibile
            ssize_t sock_client2 = find_client_by_username(server, opponent);
            send_broadcast(server, "game_not_available", game_json, client_sock, sock_client2);
            
            json_decref(request);
            break;
        case -1:
            response = create_response("accept_join", false, "La partita non esiste", NULL);
            send_json_message(response, client_sock);
            break;
        case -2:
            response = create_response("accept_join", false, "Partita non disponibile", NULL);
            send_json_message(response, client_sock);
            break;
        case -4:
            response = create_response("accept_join", false ,"Avversario impegnato in un'altra partita", NULL);
            send_json_message(response, client_sock);
            break;
        default:
            response = create_response("accept_join", false ,"Errore interno al server", NULL);
            send_json_message(response, client_sock);
            break;
    }


    json_decref(response);
    json_decref(game_json);
}

/**
 * Gestisce il caso in cui un giocatore abbandona la parita o si arrende.
 */
void handle_quit(server_t* server, const int client_sock, const json_t* data){
    json_t* response;
    size_t game_id = json_integer_value(json_object_get(data, "game_id"));
    game_t* game = find_game_by_id(server,game_id);

    if(!game){
        response = create_response("game_quit", false, "La partita non esiste", NULL);
        send_json_message(response, client_sock);
        json_decref(response);
        return;
    }

    const char* username = find_username_by_client(server, client_sock);
    short result = quit(server,game,username);

    switch(result){
        case 0:
            response = create_response("game_quit", true, "Partita abbandonata con successo", create_json(server, game->id, false));
            send_json_message(response, client_sock);

            ssize_t owner = find_client_by_username(server, game->player1);
            send_broadcast(server, "game_ended", create_json(server, game->id, false), owner , -1);
            break;
        case -1:
            response = create_response("game_quit", false, "La partita non è in corso", NULL);
            send_json_message(response, client_sock);
            break;
        case -2:
            response = create_response("game_quit", false, "Impossibile notificare l'avversario dell'abbandono", NULL);
            send_json_message(response, client_sock);
            break;
        default:
            response = create_response("game_quit", false ,"Errore interno al server", NULL);
            send_json_message(response, client_sock);
            break;
    }

    json_decref(response);
}

//============ INTERFACCIA PUBBLICA ==================//

/** 
 * Gestisce le varie richieste inviate dal client 
*/
void handle_request(server_t* server, const int client_sock, const json_t* json_request){
    const char* request = json_string_value(json_object_get(json_request, "request"));
    json_t* data = json_object_get(json_request, "data");


    if(strcmp(request, "login") == 0){
        handle_login(server, client_sock, data);
        return;
    }
    if(strcmp(request, "create_game") == 0){
        handle_create_game(server, client_sock);
        return;
    }

    if(strcmp(request, "join_request") == 0){
        handle_join_request(server, client_sock, data);
        return;
    }

    if(strcmp(request, "accept_join") == 0){
        handle_accept_join(server, client_sock, data);
        return;
    }

    if (strcmp(request, "list_games") == 0){
        handle_list_games(server, client_sock);
        return;
    }

    if (strcmp(request, "game_move") == 0){
        handle_game_move(server, client_sock, data);
        return;
    }

    if (strcmp(request, "game_quit") == 0){
        handle_quit(server, client_sock, data);
        return;
    }
    
    json_t* response = create_response("error", false, "Richiesta non valida", NULL);
    send_json_message(response, client_sock);
    json_decref(response);
}
