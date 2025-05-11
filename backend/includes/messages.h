#ifndef MESSAGES_H
#define MESSAGES_H

#include <jansson.h>
#include <stdbool.h>
#include <server.h>

#include "game.h"

#define MAX_JSON_SIZE 1048576  // 1 MB

/**
 * Invia i dati di aggiornamento della partita. 
 * Il parametro username indica chi ha effettuato la mossa, dunque invia lo stato della 
 * partita aggiornata come risposta a quest'ultimo e all'avversario una richiesta allo 
 * scopo di aggiornare i dati che ha.
 * Ritorna true se il messaggio è stato inviato correttamente ad entrambi i giocatori, false altrimenti.
 */
bool send_game_update(server_t* server, game_t* game, const char* username);

/**
 * Invia un messaggio in broadcast a tutti i client connessi esclusi exclude_client1 e exclude_client2.
 * Ritorna ture se il messaggio è stato inviato a tutti i client connessi, false altrimenti.
 */
bool send_broadcast(server_t* server, const char* event_type, json_t* data, const ssize_t exclude_client1, const ssize_t exclude_client2);

/**
 * Invia un messaggio ad uno specifico player.
 * Ritorna true se il messaggio è stato inviato correttamente, false altrimenti.
 */
bool send_to_player(server_t* server, json_t* json_data, const char* username, bool already_locked);

/**
 * Invia un messaggio Json aggiungendo un delimitatore di fine (come \n) assicurandosi che l'intero messaggio venga inviato
 * anche se la rete o la connessione non permettono di inviarlo tutto in un'unica volta.
 * Ritorna ture se il messaggio è stato inviato correttamente, false altrimenti.
 */
bool send_json_message(json_t* json_data, const size_t sock);

/**
 * Crea una richiesta standard in formato json specificando il tipo di richiesta e una descrizione.
 * Ritorna la richiesta json il caso di corretta creazione, NULL altrimenti.
 */
json_t* create_request(const char* request_type, const char* description, json_t* data);

/**
 * Crea una risposta standard in formato json specificando il tipo di risposta e una descrizione.
 * Ritorna la risposta json il caso di corretta creazione, NULL altrimenti.
 */
json_t* create_response(const char* response_type, bool status, const char* description, json_t* data);


/**
 * Gestisce la ricezione di un messaggio. 
 * Il metodo si aspetta prima la lunghezza del messaggio e successivamente il messaggio.
 * Ritorna il file json se la ricezione è avvenuta correttamente, NULL altrimenti.
 */
json_t* receive_json(const size_t socket);
#endif