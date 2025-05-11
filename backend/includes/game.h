#ifndef GAME_H
#define GAME_H

#include "jansson.h"
#include "server.h"

typedef enum {
    GAME_WAITING,
    GAME_ONGOING,
    GAME_OVER
} game_state_t;

typedef struct {
    size_t id;
    char player1[64];
    char player2[64];
    char board[3][3];
    char turn[64];
    game_state_t state;
    char winner[64];    //Vincitore oppure (game_state = GAME_OVER e winner vuoto ) se è pareggio
    unsigned short int rematch;     // 1 = player 1 vuole la rivincita, 2 = player 2 vuole la rivincita, 3 = entrambi vogliono la rivincita, 0 altrimenti
} game_t;

typedef struct{
    game_t* game;
    server_t* server;
    char username[64];
}timeout_args_t;

typedef struct game_node {
    game_t game;
    struct game_node* next;
} game_node_t;

typedef struct {
    game_node_t* head;
    size_t count;
} game_list_t;

extern game_list_t* game_list;

/**
 * Inizializza le strutture utili per gestire le partite esistenti
 */
void game_init(server_t* server);

/**
 * Libera la memoria allocata per gestire le partite esistenti 
 */
void game_cleanup(server_t* server);

/**
 * Crea una nuova partita.
 * Ritorna l'id della partita creata, altrimenti -1 in caso di errore 
 */
ssize_t create_game(server_t* server, const char* player1);

/**
 * Invia al creatore della partita la richiesta di join da parte di un utente per una determinata partita.
 * Ritorna 0 se la richiesta è avvenuta con successo, -1 se game_id non è valido, -2 se la partita non è più disponibile
 */
short request_join_game(server_t* server, size_t game_id, const char *player2);

/**
 * Invia all'avversario la notifica che la partita è stata accettata e che sta per essere avviata.
 * Ritorna 0 se l'invio è avvenuto con successo, -1 se game_id non è valido, 
 * -2 se la partita non è più disponibile, -4 se l'avversario non è più disponibile
 */
short accept_join_request(server_t* server, size_t game_id, const char* player2);

/**
 * Metodo che gestisce la mossa, quindi, aggiorna lo stato della board, verifica se è stato fatto un tris e cambia il turno.
 * Ritorna 0 se la mossa è stata fatta con successo, -1 se la parita non è nello stato GAME_ONGOING
 */
short make_move(server_t* server, game_t* game, const char *username, int x, int y);

/**
 * Cerca una partita a partire dall'id.
 * Ritorna un oggetto di tipo game_t se la partita è stata trovata, NULL altrimenti
 */
game_t* find_game_by_id(server_t* server,size_t game_id);

/**
 * Serializza la struttura game_t in json
 */
json_t* create_json(server_t* server, size_t id, bool already_locked);

/**
 * Ritorna un json con tutte le partite create da un giocatore, altrimenti NULL
 */
json_t* list_games(server_t* server, const char* username);


/**
 * Gestione abbandono partita. Notifica il player in gioco che ha l'avversario ha abbandonato e dunque ha vinto la partita.
 * Ritorna 0 se lo stato della partita e l'invio della notifica vanno a buon fine, -1 se la partita non è in corso, -2 errore invio notifica
 */
short quit(server_t* server, game_t* game, const char* username);

/**
 * Rimuove tutte le partite associate ad un giocatore
 */
void remove_games_by_username(server_t* server, const char* username, const size_t sock);
#endif