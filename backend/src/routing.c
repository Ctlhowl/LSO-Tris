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

    // Verifica unicità del nome
    if (!is_username_unique(server, username)) {
        send_json_message(create_response("login", false, "Username già in uso, riprova", NULL), client_sock);
        return;
    }

    // Crea un nuovo client
    client_t* new_client = (client_t*)malloc(sizeof(client_t));
    if (!new_client) {
        send_json_message(create_response("login", false, "Errore Server", NULL), client_sock);
        return;
    }
    
    
    memset(new_client, 0, sizeof(client_t));
    new_client->socket = client_sock;
    new_client->username[0] = '\0';  
    

    strncpy(new_client->username, username, strlen(username));

    // Aggiunge il client alla lista di client connessi
    if (!client_add(server, new_client)) {
        send_json_message(create_response("login", false, "Errore Server", NULL), client_sock);
        return;
    }

    send_json_message(create_response("login", true, "Benvenuto nel gioco", NULL), client_sock);
}

/**
 * Gestisce la creazione di un nuovo gioco.
 * Crea la partita e invia al client le informazioni relative ad essa, altrimenti lo notifica dell'errore
 */
void handle_create_game(server_t* server, const int client_sock){
    const char* username = find_username_by_client(server, client_sock);
    ssize_t game_id = create_game(server, username);

    if(game_id >= 0){
        send_json_message(create_response("create_game", true, "Partita creata con successo", NULL), client_sock);

        // Notifica tutti i client della creazione di un nuovo gioco
        send_broadcast(server, "new_game_available", create_json(server, game_id), client_sock, -1);
        return;
    }

    send_json_message(create_response("create_game", false, "Errore interno al server", NULL), client_sock);
}

/**
 * Ricerca ed invia al client tutte le partite presenti tranne quelle create da se stesso.
 */
void handle_list_games(server_t* server, const int client_sock){
    const char* username = find_username_by_client(server, client_sock);
    json_t* games = list_games(server, username);

    if(games){
        send_json_message(create_response("list_games", true, "Lista delle partite disponibili", games), client_sock);
        return;
    }

    send_json_message(create_response("list_games", false, "Errore nel recupero delle partite", NULL), client_sock);
}

/**
 * Gestisce la richiesta per unirsi.
 * Viene creata e inviata la richiesta di unione al creatore della partita e notifica il richiedente se:
 * 1 - La richiesta è stata inviata correttamente
 * 2 - Id della partita non esiste
 * 3 - La partita non è più disponibile
 * 4 - Errore generico lato server
 */
void handle_join_request(server_t* server, const int client_sock, const json_t* data){
    const char* username = find_username_by_client(server, client_sock);
    ssize_t game_id = json_integer_value(json_object_get(data, "game_id"));

    if (game_id >= 0) {
        short result = request_join_game(server, game_id, username);

        switch(result){
            case 0:
                send_json_message(create_response("join_request", true, "Richiesta inviata correttamente", NULL), client_sock);
                break;;
            case -1:
                send_json_message(create_response("join_request", false, "Id partita inesistente", NULL), client_sock);
                break;
            case -2:
                send_json_message(create_response("join_request", false, "La parita non esiste più", NULL), client_sock);
                break;
            default:
                send_json_message(create_response("join_request", false, "Errore Server", NULL), client_sock);
                break;
        }
    }
}

/**
 * Gestisce se la mossa effettuata è valida rispetto al turno e la cella.
 */
void handle_game_move(server_t* server, const int client_sock, const json_t* data){

    size_t game_id = json_integer_value(json_object_get(data, "game_id"));
    short x = json_integer_value(json_object_get(data, "x"));
    short y = json_integer_value(json_object_get(data, "y"));

    game_t* game = find_game_by_id(server,game_id);

    if(!game){
        send_json_message(create_response("game_move", false, "Partita non trovata", NULL), client_sock);
        return;
    }

    const char* username = find_username_by_client(server, client_sock);
    short result = make_move(server, game, username, x, y);

    switch(result){
        case 0 :
            send_game_update(server,game, username);
            break;
        case -1:
            send_json_message(create_response("game_move", false, "La partita non è in gioco",NULL),client_sock);
            break;
        case -2:
            send_json_message(create_response("game_move", false, "Attendi che sia il tuo turno per effettuare la mossa",NULL),client_sock);
            break;
        case -3:
            send_json_message(create_response("game_move", false, "Cella già occupata",NULL),client_sock);
            break;
        default:
            send_json_message(create_response("game_move", false, "Errore interno al server",NULL), client_sock);
            break;;
    }

}

/**
 * Gestione caso in cui il creatore della partita accetta la richiesta di join da parte di client_sock.
 */
void handle_accept_join(server_t* server, const int client_sock, const json_t* data){
    size_t game_id = json_integer_value(json_object_get(data, "game_id"));
    const char* opponent = json_string_value(json_object_get(data, "player2"));

    short result = accept_join_request(server, game_id, opponent);
    
    switch(result){
        case 0:
            json_t* game_param = create_json(server, game_id);

            // Notifica il creatore che la partita sta stata accettata con successo e può essere avviata
            send_json_message(create_response("accept_join", true, "Richiesta accettata con successo", game_param), client_sock);
            send_json_message(create_request("game_started", "La partita sta per cominciare", game_param), client_sock);

            // Notifica tutti i client che il game con id game_id non é piu disponibile
            ssize_t sock_client2 = find_client_by_username(server, opponent);
            send_broadcast(server, "game_not_available", create_json(server, game_id), client_sock, sock_client2);
            break;
        case -1:
            send_json_message(create_response("accept_join", false, "La partita non esiste", NULL), client_sock);
            break;
        case -2:
            send_json_message(create_response("accept_join", false, "Partita non disponibile", NULL), client_sock);
            break;
        case -4:
            send_json_message(create_response("accept_join", false ,"Avversario impegnato in un'altra partita", NULL), client_sock);
            break;
        default:
            send_json_message(create_response("accept_join", false ,"Errore interno al server", NULL), client_sock);
            break;;
    }
}

/**
 * Gestisce il caso in cui un giocatore abbandona la parita o si arrende.
 */
void handle_quit(server_t* server, const int client_sock, const json_t* data){
    size_t game_id = json_integer_value(json_object_get(data, "game_id"));
    game_t* game = find_game_by_id(server,game_id);

    if(!game){
        send_json_message(create_response("game_quit", false, "La partita non esiste", NULL), client_sock);
        return;
    }

    const char* username = find_username_by_client(server, client_sock);
    short result = quit(server,game,username);

    switch(result){
        case 0:
            json_t* game_param = create_json(server, game->id);
            send_json_message(create_response("game_quit", true, "Partita abbandonata con successo", game_param), client_sock);

            ssize_t owner = find_client_by_username(server, game->player1);
            send_broadcast(server, "game_ended", game_param, owner , -1);
            break;
        case -1:
            send_json_message(create_response("game_quit", false, "La partita non è in corso", NULL), client_sock);
            break;
        case -2:
            send_json_message(create_response("game_quit", false, "Impossibile notificare l'avversario dell'abbandono", NULL), client_sock);
            break;
        default:
            send_json_message(create_response("game_quit", false ,"Errore interno al server", NULL), client_sock);
            return;
    }
}

/**
 * Gestisce il caso in cui uno, entrambi o nessun giocatore voglia fare il rematch
 */
void handle_rematch(server_t* server, const int client_sock, const char* username, const json_t* data){
    size_t game_id = json_integer_value(json_object_get(data, "game_id"));
    game_t* game = find_game_by_id(server,game_id);

    if(!game){
        send_json_message(create_response("game_rematch", false, "La partita non esiste", NULL), client_sock);
        return;
    }

    short result = rematch(server, game, username);

    switch (result){
    case 0:
        send_json_message(create_response("game_resetted", true, "Partita resettata ed in attesa", create_json(server,game->id)), client_sock);
        send_broadcast(server, "new_game_available", create_json(server, game_id), client_sock, -1);
        break;
    case 1:
        send_json_message(create_response("game_rematch", true, "Rivincita avvenuta con successo", NULL), client_sock);
        break;
    case 2:
        send_json_message(create_response("waiting_opponent", true, "In attesa dell'avversario", NULL), client_sock);
        break;
    default:
        break;
    }

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

    if (strcmp(request, "rematch") == 0){
        
        return;
    }else{
        send_json_message(create_response("error", false, "Richiesta non valida", NULL), client_sock);
        return;
    }
}
