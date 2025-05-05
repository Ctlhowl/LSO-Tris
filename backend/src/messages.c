#include "messages.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"
#include "game.h"

//============ METODI PRIVATI ==================//

/**
 * Garantisce che tutti i dati vengano inviati correttamente su un socket, anche quando la funzione send() non invia tutto in una sola volta
 * Dopo ogni chiamata a send() la funzione verifica quanti byte sono stati effettivamente inviati.
 * Se non sono stati inviati tutti i byte, continua a inviare i dati rimanenti
*/
bool send_all_bytes(const int sock, const void *data, const size_t length) {
    size_t total_sent = 0;
    const char* buffer = (const char*)data;
    while (total_sent < length) {
        ssize_t sent = send(sock, buffer + total_sent, length - total_sent, 0);
        if (sent <= 0) return false;
        total_sent += sent;
    }
    return true;
}


//============ INTERFACCIA PUBBLICA ==================//


bool send_game_update(server_t* server, game_t* game, const char* username){
    json_t* response = json_object();
    json_t* request = json_object();

    json_t* game_param = create_json(server, game->id);

    pthread_mutex_lock(&server->games_mutex);

    if(game->state == GAME_ONGOING){
       response = create_response("game_move", true, "La partita è ancora in corso", game_param);
       request = create_request("game_update", "La partita è ancora in corso", game_param);
    } else if(game->state == GAME_OVER && game->winner[0] == '\0'){
        response = create_response("game_move", true, "Partita finita con pareggio", game_param);
        request = create_request("game_update", "Partita finita con pareggio", game_param);
    } else if (game->state == GAME_OVER){
        response = create_response("game_move", true, "Partita finita con vincitore", game_param);   
        request = create_request("game_update", "Partita finita con vincitore", game_param);
    }

    bool sendedToPlayer1 = false;
    bool sendedToPlayer2 = false;

    ssize_t sock_client1 = find_client_by_username(server,game->player1);
    ssize_t sock_client2 = find_client_by_username(server,game->player2);

    if(strcmp(game->player1, username) == 0){
        sendedToPlayer1 = send_json_message(response, sock_client1);
        sendedToPlayer2 = send_json_message(request, sock_client2);
    }else{
        sendedToPlayer1 = send_json_message(request, sock_client1);
        sendedToPlayer2 = send_json_message(response, sock_client2);
    }
    
    if(sendedToPlayer1 && sendedToPlayer2){
        pthread_mutex_unlock(&server->games_mutex);
        return true;
    }

    pthread_mutex_unlock(&server->games_mutex);
    perror("failed to send game updates");
    return false;
}

/**
 * Invia un messaggio in broadcast a tutti i client connessi escluso uno
 */
bool send_broadcast(server_t* server, json_t* json_data, const int exclude_client1, const int exclude_client2) {
    if (!json_data) return false;

    pthread_mutex_lock(&server->clients_mutex);
    
    // Invio messaggi a tutti i client
    client_node_t* current = connected_clients->head;
    while (current) {
        int sock = current->client.socket;

        bool exclude = (sock == exclude_client1) || 
        (exclude_client2 != -1 && sock == exclude_client2);

        if (!exclude) {
            if (!send_json_message(json_data, sock)) {
                perror("broadcast send failed");
                pthread_mutex_unlock(&server->clients_mutex);
                return false;
            }
        }

        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
    free(json_data);
    return true;
}

/**
 * Invia un messaggio ad uno specifico player
 */
bool send_to_player(server_t* server, json_t* json_data, const char* username) {
    if (!username || !json_data) return false;

    pthread_mutex_lock(&server->clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (strcmp(current->client.username, username) == 0) {
            int sock = current->client.socket;
            if (!send_json_message(json_data, sock)) {
                perror("send_to_player failed");

                pthread_mutex_unlock(&server->clients_mutex);
                free(json_data);
                return false;
            }

            break;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
    free(json_data);
    return true;
}


/**
 * Invia un messaggio JSON aggiungendo un delimitatore di fine (come \n) assicurandosi che l'intero messaggio venga inviato
 * anche se la rete o la connessione non permettono di inviarlo tutto in un'unica volta.
 * Ritorna 0 se il messaggio è stato inviato correttamente, -1 altrimenti.
 */
bool send_json_message(json_t* json_data, const int sock) {
    if (!json_data || sock < 0) return false;

    // Serializzazione il JSON
    char* json_str = json_dumps(json_data, JSON_COMPACT);
    if (!json_str) return false;

    //Calcolo della lunghezza (size_t in network byte order)
    uint32_t json_len = (uint32_t)strlen(json_str);
    uint32_t net_len = htonl(json_len);

    //Invio lunghezza del messaggio
    if (!send_all_bytes(sock, &net_len, sizeof(net_len))) {
        free(json_str);
        return false;
    }

    //Invio del JSON
    int result = send_all_bytes(sock, json_str, json_len);
    free(json_str);
    return result;
}

/**
 * Crea una richiesta standard in formato json specificando il tipo di richiesta e una descrizione
 */
json_t* create_request(const char* request_type, const char* description, json_t* game_param) {
    json_t* msg = json_object();
    if (!msg){
        perror("json creation failed");
        return NULL;
    }

    json_object_set_new(msg, "type", json_string("request"));
    json_object_set_new(msg, "request", json_string(request_type));
    json_object_set_new(msg, "description", json_string(description));
    
    if(game_param){
        json_object_set_new(msg, "data", game_param);
    }
    
    return msg;
}

/**
 * Crea una risposta standard in formato json specificando il tipo di risposta e una descrizione
 */
json_t* create_response(const char* response_type, bool status, const char* description, json_t* data) {
    json_t* msg = json_object();
    if (!msg){
        perror("json creation failed");
        return NULL;
    }

    json_object_set_new(msg, "type", json_string("response"));
    json_object_set_new(msg, "response", json_string(response_type));

    status ? json_object_set_new(msg, "status", json_string("ok")) : json_object_set_new(msg, "status", json_string("error")); 
    
    json_object_set_new(msg, "description", json_string(description));
    
    if(data){
        json_object_set_new(msg, "data", data);
    }

    return msg;
}


/**
 * Crea un messaggio evento 
 */
 json_t* create_broadcast(const char* event_type, json_t* game_param) {
    json_t* msg = json_object();
    if (!msg){
        perror("json creation failed");
        return NULL;
    }

    json_object_set_new(msg, "type", json_string("broadcast"));
    json_object_set_new(msg, "event", json_string(event_type));
    if(game_param){
        json_object_set_new(msg, "data", game_param);
    }

    return msg;
}

json_t* receive_json(const int socket_fd) {
    uint32_t net_len;
    if (recv(socket_fd, &net_len, sizeof(net_len), MSG_WAITALL) != sizeof(net_len)) {
        return NULL;
    }

    size_t len = ntohl(net_len);
    if (len == 0 || len > MAX_JSON_SIZE) {
        return NULL;
    }

    char* json_str = malloc(len + 1);
    if (!json_str) return NULL;

    if (recv(socket_fd, json_str, len, MSG_WAITALL) != (ssize_t)len) {
        free(json_str);
        return NULL;
    }

    json_str[len] = '\0';

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    free(json_str);

    return root;

    /*
    char buffer[1024];
    ssize_t len = recv(socket_fd, buffer, sizeof(buffer)-1, 0);
    if (len <= 0) return NULL;
    
    buffer[len] = '\0';
    
    json_error_t error;
    json_t *root = json_loads(buffer, 0, &error);

    if(!root) return NULL;

    return root;
    */
}