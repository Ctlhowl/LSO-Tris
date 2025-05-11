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
bool send_all_bytes(const int sock, const void* msg, const size_t length) {
    size_t total_sent = 0;
    const char* buffer = (const char*)msg;
    while (total_sent < length) {
        ssize_t sent = send(sock, buffer + total_sent, length - total_sent, 0);
        if (sent <= 0){
            return false;
        } 

        total_sent += sent;
    }
    
    return true;
}

/**
 * Crea un messaggio broadcast standard in formato json  
 */
 json_t* create_broadcast(const char* event_type, json_t* data) {
    json_t* msg = json_object();
    if (!msg){
        printf("[Errore - messages.create_broadcast] Creazione del messaggio Json per il broadcast fallita");
        return NULL;
    }

    json_object_set_new(msg, "type", json_string("broadcast"));
    json_object_set_new(msg, "event", json_string(event_type));
    if(data){
        json_object_set_new(msg, "data", data);
    }

    return msg;
}

//============ INTERFACCIA PUBBLICA ==================//

/**
 * Invia i dati di aggiornamento della partita. 
 * Il parametro username indica chi ha effettuato la mossa, dunque invia lo stato della 
 * partita aggiornata come risposta a quest'ultimo e all'avversario una richiesta allo 
 * scopo di aggiornare i dati che ha.
 * Ritorna true se il messaggio è stato inviato correttamente ad entrambi i giocatori, false altrimenti.
 */
bool send_game_update(server_t* server, game_t* game, const char* username){
    json_t* response;
    json_t* request;
    
    pthread_mutex_lock(&server->games_mutex);    
    if(game->state == GAME_ONGOING){
       response = create_response("game_move", true, "La partita è ancora in corso", create_json(server, game->id, true));
       request = create_request("game_update", "La partita è ancora in corso", create_json(server, game->id, true));

    } else if(game->state == GAME_OVER){
        ssize_t owner = find_client_by_username(server, game->player1);
        send_broadcast(server, "game_ended", create_json(server, game->id, true), owner , -1);

        if(game->winner[0] == '\0'){
            response = create_response("game_move", true, "Partita finita con pareggio", create_json(server, game->id, true));
            request = create_request("game_update", "Partita finita con pareggio", create_json(server, game->id, true));
        
        }else{
            response = create_response("game_move", true, "Partita finita con vincitore", create_json(server, game->id, true));   
            request = create_request("game_update", "Partita finita con vincitore", create_json(server, game->id, true));
        }
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
    
    json_decref(request);
    json_decref(response);

    if(sendedToPlayer1 && sendedToPlayer2){
        pthread_mutex_unlock(&server->games_mutex);
        printf("[Info - messages.send_game_update] I dati di aggiornamento della partita sono stati inviati correttamente\n");
        return true;
    }

    pthread_mutex_unlock(&server->games_mutex);
    printf("[Errore - messages.send_game_update] Invio dei dati di aggiornamento  della partita fallito\n");
    return false;
}

/**
 * Invia un messaggio in broadcast a tutti i client connessi esclusi exclude_client1 e exclude_client2.
 * Ritorna ture se il messaggio è stato inviato a tutti i client connessi, false altrimenti.
 */
bool send_broadcast(server_t* server, const char* event_type, json_t* data, const ssize_t exclude_client1, const ssize_t exclude_client2) {
    if (!data || strlen(event_type) == 0) {
        printf("[Errore - messages.send_broadcast] Il messaggio o il tipo di evento è vuoto\n");
        return false;
    };

    pthread_mutex_lock(&server->clients_mutex);
    
    // Invio messaggi a tutti i client
    client_node_t* current = connected_clients->head;
    while (current) {
        ssize_t sock = current->client.socket;

        bool exclude = (sock == exclude_client1) || (exclude_client2 != -1 && sock == exclude_client2);

        if (!exclude) {
            send_json_message(create_broadcast(event_type, data), sock);
        }

        current = current->next;
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
    printf("[Info - messages.send_broadcast] Messaggi inviati correttamente\n");
    return true;
}

/**
 * Invia un messaggio ad uno specifico player.
 * Ritorna true se il messaggio è stato inviato correttamente, false altrimenti.
 */
bool send_to_player(server_t* server, json_t* json_data, const char* username, bool already_locked) {
    if (!username || !json_data) return false;

    if (!already_locked) pthread_mutex_lock(&server->clients_mutex);
    
    client_node_t* current = connected_clients->head;
    while (current) {
        if (strcmp(current->client.username, username) == 0) {
            int sock = current->client.socket;
            if (!send_json_message(json_data, sock)) {
                printf("[Errore - messages.send_to_player] Invio messaggio al player %s fallito\n", username);

                if (!already_locked) pthread_mutex_unlock(&server->clients_mutex);
                free(json_data);
                return false;
            }

            break;
        }
        current = current->next;
    }
    
    if (!already_locked) pthread_mutex_unlock(&server->clients_mutex);
    free(json_data);

    printf("[Info - messages.send_to_player] Invio messaggio al player %s riuscito\n", username);
    return true;
}


/**
 * Invia un messaggio Json aggiungendo un delimitatore di fine (come \n) assicurandosi che l'intero messaggio venga inviato
 * anche se la rete o la connessione non permettono di inviarlo tutto in un'unica volta.
 * Ritorna ture se il messaggio è stato inviato correttamente, false altrimenti.
 */
bool send_json_message(json_t* json_data, const size_t sock) {
    if (!json_data) return false;

    // Serializzazione il JSON
    char* json_str = json_dumps(json_data, JSON_COMPACT);
    if (!json_str){
        printf("[Errore - messages.send_json_message] Errore serializzazione del messaggio json\n");
        return false;
    }

    //Calcolo della lunghezza (size_t in network byte order)
    uint32_t json_len = (uint32_t)strlen(json_str);
    uint32_t net_len = htonl(json_len);

    //Invio lunghezza del messaggio
    if (!send_all_bytes(sock, &net_len, sizeof(net_len))) {
        printf("[Errore - messages.send_json_message] Errore invio lunghezza messaggio\n");
        free(json_str);
        return false;
    }

    //Invio del JSON
    if(!send_all_bytes(sock, json_str, json_len)){
        printf("[Errore - messages.send_json_message] Errore invio messaggio\n");
    }

    printf("[Info - messages.send_json_message] Messaggio inviato correttamente al client %ld: %s\n", sock, json_str);
    free(json_str);
    return true;
}

/**
 * Crea una richiesta standard in formato json specificando il tipo di richiesta e una descrizione.
 * Ritorna la richiesta json il caso di corretta creazione, NULL altrimenti.
 */
json_t* create_request(const char* request_type, const char* description, json_t* data) {
    json_t* msg = json_object();
    if (!msg){
        printf("[Errore - messages.create_request] Creazione della richiesta Json fallita");
        return NULL;
    }

    json_object_set_new(msg, "type", json_string("request"));
    json_object_set_new(msg, "request", json_string(request_type));
    json_object_set_new(msg, "description", json_string(description));
    
    if(data){
        json_object_set_new(msg, "data", data);
    }
    
    return msg;
}

/**
 * Crea una risposta standard in formato json specificando il tipo di risposta e una descrizione.
 * Ritorna la risposta json il caso di corretta creazione, NULL altrimenti.
 */
json_t* create_response(const char* response_type, bool status, const char* description, json_t* data) {
    json_t* msg = json_object();
    if (!msg){
        printf("[Errore - messages.create_response] Creazione della risposta Json fallita");
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
 * Gestisce la ricezione di un messaggio. 
 * Il metodo si aspetta prima la lunghezza del messaggio e successivamente il messaggio
 * Ritorna il file json se la ricezione è avvenuta correttamente, NULL altrimenti.
 */
json_t* receive_json(const size_t socket) {
    uint32_t net_len;
    if (recv(socket, &net_len, sizeof(net_len), MSG_WAITALL) != sizeof(net_len)) {
        printf("[Errore - messages.receive_json] Ricezione della lunghezza del messaggio fallita\n");
        return NULL;
    }

    size_t len = ntohl(net_len);
    if (len == 0 || len > MAX_JSON_SIZE) {
        printf("[Errore - messages.receive_json] Lunghezza del messaggio non valida, deve essere compresa tra 1 e %d bytes", MAX_JSON_SIZE);
        return NULL;
    }

    char* json_str = malloc(len + 1);
    if (!json_str){
        printf("[Errore - messages.receive_json] Impossibile allocare memoria per il messaggio ricevuto\n");
        return NULL;
    }

    if (recv(socket, json_str, len, MSG_WAITALL) != (ssize_t)len) {
        printf("[Errore - messages.receive_json] Ricezione del messaggio fallita\n");
        free(json_str);
        return NULL;
    }

    json_str[len] = '\0';

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    free(json_str);

    return root;
}