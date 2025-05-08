#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <jansson.h>
#include <pthread.h>
#include <stdbool.h>

#include "messages.h"
#include "client.h"

game_list_t* game_list = NULL;

//============ METODI PRIVATI ==================//
/**
 * Alloca un nuovo nodo per la struttura game_list_t
 * Ritorna un nodo di tipo game_t se è stato possibile allocare memoria, NULL altrimenti
 */
game_t* new_node(const char* player1){
    game_t* new_game = (game_t*)malloc(sizeof(game_t));
    if(!new_game){
        printf("[Errore - game.new_node] - Impossibile allocare memoria per una partita\n");
        return NULL;
    }

    new_game->id = game_list->count;
    strncpy(new_game->player1, player1, sizeof(new_game->player1));
    new_game->player2[0] = '\0';

    memset(new_game->board, 0, sizeof(new_game->board));
    
    strncpy(new_game->turn, player1, sizeof(new_game->turn));
    new_game->state = GAME_WAITING;
    new_game->winner[0] = '\0' ; 
    new_game->rematch = -1;

    return new_game;
}

/**
 * Aggiunge una partita alla lista delle partite presenti nel server
 * Ritorna true se il client è stato aggiunto correttamente, false altrimenti
 */
bool game_add(server_t* server, game_t* game){
    pthread_mutex_lock(&server->games_mutex);
    game_node_t* new_node = (game_node_t*)malloc(sizeof(game_node_t));
    if (!new_node) {
        printf("[Errore - game.game_add] - Impossibile allocare memoria per aggiungere una partita alla lista delle partite presenti\n");
        pthread_mutex_unlock(&server->games_mutex);
        return false;
    }
    
    new_node->game = *game;
    new_node->next = game_list->head;
    game_list->head = new_node;
    game_list->count++;
    
    pthread_mutex_unlock(&server->games_mutex);
    return true;
}

/**
 * Rimuove una partita dalla lista delle partite presenti nel server
 * Ritorna true se il client è stato aggiunto correttamente, false altrimenti
 */
bool game_remove(server_t* server, const size_t id) {  
    pthread_mutex_lock(&server->games_mutex);

    game_node_t** pp = &game_list->head;
    game_node_t* current = game_list->head;
    
    while (current) {
        if (current->game.id == id) {
            *pp = current->next;
            free(current);

            game_list->count--;
            
            pthread_mutex_unlock(&server->games_mutex);
            return true;
        }
        pp = &current->next;
        current = current->next;
    }
    
    printf("[Errore - client.client_remove] Il client che si sta cercando di rimuovere non esiste\n");
    pthread_mutex_unlock(&server->games_mutex);
    return false;
}

/**
 * Casta l'enum in stringa
 */
const char* game_state_to_string(game_state_t state) {
    switch (state) {
        case GAME_WAITING: return "GAME_WAITING";
        case GAME_ONGOING: return "GAME_ONGOING";
        case GAME_OVER:    return "GAME_OVER";
        default:           return "unknown";
    }
}

/**
 * Verifica se l'avversario è ancora disponibile per giocare la partita.
 * Ritorna true se è ancora disponibile, false altrimenti
 */
bool is_opponent_available(server_t* server, const char* player2){
    pthread_mutex_lock(&server->games_mutex);

    game_node_t *curr = game_list->head;
    
    while(curr){
        game_t game = curr->game;
        if (game.state == GAME_ONGOING){
            if(strcmp(game.player1, player2) == 0 || strcmp(game.player2, player2) == 0){
                printf("[Info - game.is_opponent_available] %s è gia impegnato in un'altra partita\n", player2);
                
                pthread_mutex_unlock(&server->games_mutex);
                return false;
            }
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&server->games_mutex);
    return true;
}


/**
 * Verifica se è stato fatto un tris.
 * Ritorna 1 se è stato fatto tris, 0 pareggio, -1 se la partita è ancora in corso
 */
short check_tris(game_t* game){
    // Controlla righe, colonne e diagonali per una vittoria
    for (int i = 0; i < 3; i++) {
        // Controllo righe
        if (game->board[i][0] != '\0' && game->board[i][0] == game->board[i][1] && game->board[i][1] == game->board[i][2]) {
            return 1;  // Vincitore
        }
        // Controllo colonne
        if (game->board[0][i] != '\0' && game->board[0][i] == game->board[1][i] && game->board[1][i] == game->board[2][i]) {
            return 1;  // Vincitore
        }
    }

    // Controllo diagonali
    if (game->board[0][0] != '\0' && game->board[0][0] == game->board[1][1] && game->board[1][1] == game->board[2][2]) {
        return 1;  // Vincitore
    }
    if (game->board[0][2] != '\0' && game->board[0][2] == game->board[1][1] && game->board[1][1] == game->board[2][0]) {
        return 1;  // Vincitore
    }

    // Controlla pareggio
    bool filled = true;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (game->board[i][j] == '\0') {
                filled = false;
                break;
            }
        }
    }

    if (filled) {
        game->state = GAME_OVER;
        return 0;  // Pareggio
    }

    return -1;  // Continua a giocare
}

/**
 * Reimposta i dati della partita a seconda del parametro choice.
 * Choice = 3:
 * Choice = 2: 
 */
void reset_game(server_t* server, game_t* game, unsigned short choice){
    pthread_mutex_lock(&server->games_mutex);

    if(choice == 3){
        memset(game->board,0,sizeof(game->board));
        strncpy(game->turn, game->player1, sizeof(game->turn));
        game->state = GAME_ONGOING;
        game->winner[0] = '\0';
        game->rematch = 0;
        pthread_mutex_unlock(&server->games_mutex);
        return;
    }

    if(choice == 2){
        strncpy(game->player1, game->player2, sizeof(game->player1));
    }

    game->player2[0] = '\0';
    memset(game->board,0,sizeof(game->board));
    strncpy(game->turn, game->player1, sizeof(game->turn));
    game->state = GAME_WAITING;
    game->winner[0] = '\0';
    game->rematch = 0;
    pthread_mutex_unlock(&server->games_mutex);
}

//============ INTERFACCIA PUBBLICA ==================//

/**
 * Inizializza le strutture utili per gestire le partite esistenti
 */
void game_init(server_t* server) {
    pthread_mutex_lock(&server->games_mutex);

    if (game_list == NULL) {
        game_list = (game_list_t*)malloc(sizeof(game_list_t));
        if (!game_list) {
            printf("[Errore - game.game_init] Impossibile allocare memoria per la lista delle partite\n");
            exit(EXIT_FAILURE);
        }
        
        game_list->head = NULL;
        game_list->count = 0;
    }

    pthread_mutex_unlock(&server->games_mutex);
}

/**
 * Libera la memoria allocata per gestire le partite esistenti 
 */
void game_cleanup(server_t* server) {
    pthread_mutex_lock(&server->games_mutex);
    
    game_node_t* current = game_list->head;
    while (current) {
        game_node_t* next = current->next;
        free(current);
        current = next;
    }
    
    free(game_list);
    game_list = NULL;
    
    pthread_mutex_unlock(&server->games_mutex);
}

/**
 * Crea una nuova partita.
 * Ritorna l'id della partita creata, altrimenti -1 in caso di errore 
 */
ssize_t create_game(server_t* server, const char* player1) {
    pthread_mutex_lock(&server->games_mutex);

    // Controllo disponibilità slot partite 
    if(game_list->count >= MAX_GAMES){
        pthread_mutex_unlock(&server->games_mutex);
        printf("[Errore - game.create_game] Impossibile creare una partita il server è al momento pieno\n");
        return -1;
    }

    game_t* new_game = new_node(player1);
    if(!new_game){
        pthread_mutex_unlock(&server->games_mutex);
        return -1;
    }
    pthread_mutex_unlock(&server->games_mutex);

    if(!game_add(server, new_game)){
        return -1;
    }
    
    return new_game->id;
}

/**
 * Rimuove la partita dalla lista di partite disponibili nel server.
 * Ritorna true se la partita è stata rimossa correttamente, false altrimenti 
 */
bool remove_game(server_t* server, size_t game_id) {
    pthread_mutex_lock(&server->games_mutex);
    
    game_node_t** pp = &game_list->head;
    game_node_t* current = game_list->head;
    
    while (current) {
        if (current->game.id == game_id) {
            *pp = current->next;
            
            printf("[Info - game.remove_game] La partita con id %ld è terminata)\n", game_id);
            free(current);

            game_list->count--;

            pthread_mutex_unlock(&server->games_mutex);
            return true;
        }
        pp = &current->next;
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->games_mutex);

    printf("[Errore - game.remove_game] La partita con id %ld non esiste\n", game_id);
    return false;
}

/**
 * Invia al creatore della partita la richiesta di join da parte di un utente per una determinata partita.
 * Ritorna 0 se la richiesta è avvenuta con successo, -1 se game_id non è valido, -2 se la partita non è più disponibile
 */
short request_join_game(server_t* server, size_t game_id, const char *player2) {
    if (game_id >= game_list->count) {
        printf("[Errore - game.request_join_game] Id partita inesistente\n");
        return -1;
    }

    pthread_mutex_lock(&server->games_mutex);
    game_node_t *curr = game_list->head;
    
    while(curr){
        if(curr->game.id == game_id){

            // Verifica che la partita sia in stato di "attesa"
            if (curr->game.state != GAME_WAITING) {
                pthread_mutex_unlock(&server->games_mutex);
                
                printf("[Errore - game.request_join_game] La parita non esiste più\n");
                return -2;
            }

            // Invia la richiesta di join al creatore della partita (player1)
            json_t *data = json_object();
            json_object_set_new(data, "game_id", json_integer(game_id));
            json_object_set_new(data, "player2", json_string(player2));

            send_to_player(server, create_request("join_request", "Nuova richiesta di join", data), curr->game.player1);

            pthread_mutex_unlock(&server->games_mutex);
            return 0;
        }

        curr = curr->next;
    }

    pthread_mutex_unlock(&server->games_mutex);
    printf("[Errore - game.request_join_game] Id partita inesistente\n");
    return -1; 
   
}

/**
 * Invia all'avversario la notifica che la partita è stata accettata e che sta per essere avviata.
 * Ritorna 0 se l'invio è avvenuto con successo, -1 se game_id non è valido, 
 * -2 se la partita non è più disponibile, -4 se l'avversario non è più disponibile
 */
short accept_join_request(server_t* server, size_t game_id, const char *player2){
    if (game_id >= game_list->count) {
        printf("[Errore - game.accept_join_request] Id partita inesistente\n");
        return -1;
    }

    pthread_mutex_lock(&server->games_mutex);
    game_node_t *curr = game_list->head;
    
    while(curr){
        game_t *game = &curr->game;
        if(game->id == game_id){
            
            // Verifica che la partita sia in stato di "attesa"
            if (game->state != GAME_WAITING) {
                pthread_mutex_unlock(&server->games_mutex);
                printf("[Errore - game.accept_join_request] La parita non esiste più\n");
                return -2;
            }
            
            pthread_mutex_unlock(&server->games_mutex);


            // Verifico se l'avversario é impegnato in un'altra partita
            if(!is_opponent_available(server, player2)){
                printf("[Errore - game.accept_join_request] Avversario impegnato in un'altra partita\n");
                return -4; 
            }

            // Aggiungi il secondo giocatore alla partita
            strncpy(game->player2, player2, sizeof(game->player2) - 1);
            game->state = GAME_ONGOING;

            json_t* data = create_json(server, game->id);
            
             // Notifica l'avversario che la partita sta stata accettata con successo e che può essere avviata
            send_to_player(server, create_request("accept_join", "Richiesta accettata", NULL), curr->game.player2);
            send_to_player(server, create_request("game_started", "La partita sta per cominciare", data), curr->game.player2);

            return 0;
        }

        curr = curr->next;
    }

    pthread_mutex_unlock(&server->games_mutex);
    return -1;  // Partita non trovata
}

/**
 * Metodo che gestisce la mossa, quindi, aggiorna lo stato della board, verifica se è stato fatto un tris e cambia il turno.
 * Ritorna 0 se la mossa è stata fatta con successo, -1 se la parita non è nello stato GAME_ONGOING
 */
short make_move(server_t* server, game_t* game, const char *username, int x, int y) {
    pthread_mutex_lock(&server->games_mutex);
    
    if(game->state != GAME_ONGOING){
        pthread_mutex_unlock(&server->games_mutex);
        printf("[Errore - game.make_move] La partita non è stata ancora avviata\n");
        return -1;
    }

    // Verifica che sia il turno del giocatore che ha effettuato la mossa
    if (strcmp(game->turn, username) != 0) {
        pthread_mutex_unlock(&server->games_mutex);
        printf("[Errore - game.make_move] Non è il turno del giocatore %s\n", username);
        return -2;
    }

    // Esegui la mossa
    if(game->board[x][y] != '\0'){
        pthread_mutex_unlock(&server->games_mutex);
        printf("[Errore - game.make_move] Cella già occupata\n");
        return -3;
    }

    char symbol = (strcmp(game->turn, game->player1) == 0) ? 'X' : 'O';
    game->board[x][y] = symbol;

    switch (check_tris(game)){
        case -1:
            // Cambia il turno
            strncpy(game->turn, (strcmp(game->turn, game->player1) == 0) ? game->player2 : game->player1, 63);
            break;
        case 0:
            // Fine partita in pareggio 
            game->state = GAME_OVER;
            break;
        case 1:
            // Fine partita con vincitore
            game->state = GAME_OVER;
            strncpy(game->winner, username, 63); // Imposta il vincitore
            break;
    }

    pthread_mutex_unlock(&server->games_mutex);
    return 0;
}


/**
 * Cerca una partita a partire dall'id.
 * Ritorna un oggetto di tipo game_t se la partita è stata trovata, NULL altrimenti
 */
game_t* find_game_by_id(server_t* server,size_t game_id){
    pthread_mutex_lock(&server->games_mutex);
    game_node_t *curr = game_list->head;
    
    while(curr){
        game_t* game = &curr->game;
        if (game->id == game_id){
            pthread_mutex_unlock(&server->games_mutex);
            return game;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&server->games_mutex);
    return NULL;
}

/**
 * Serializza la struttura game_t in json
 */
json_t* create_json(server_t* server, size_t id){
    json_t* msg = json_object();
    if (!msg) return NULL;
    
    pthread_mutex_lock(&server->games_mutex);
    
    game_node_t* current = game_list->head;
    game_t* found_game = NULL;
    
    // Cerca il gioco
    while (current && !found_game) {
        if (current->game.id == id) {
            found_game = &current->game;
            break;

        }
        current = current->next;
    }
    
    if (!found_game) {
        pthread_mutex_unlock(&server->games_mutex);
        return NULL;
    }
    
    // Serializza i dati del gioco
    json_object_set_new(msg, "game_id", json_integer(found_game->id));
    json_object_set_new(msg, "player1", json_string(found_game->player1));
    
    // Gestione player2
    if (strlen(found_game->player2) > 0) {
        json_object_set_new(msg, "player2", json_string(found_game->player2));
    } else {
        json_object_set_new(msg, "player2", json_null());
    }
    
    // Serializzazione board
    json_t *json_board = json_array();
    if (!json_board){
        pthread_mutex_unlock(&server->games_mutex);
        return NULL;
    }
    
    for (int i = 0; i < 3; i++) {
        json_t *row = json_array();
        if (!row){
            pthread_mutex_unlock(&server->games_mutex);
            return NULL;
        }
        
        for (int j = 0; j < 3; j++) {
            char cell[2] = { found_game->board[i][j] == ' ' ? '\0' : found_game->board[i][j], '\0' };
            json_t* cell_json = json_string(cell);
            if (!cell_json || json_array_append_new(row, cell_json) != 0) {
                if (cell_json){
                    pthread_mutex_unlock(&server->games_mutex);
                    return NULL;
                }
            }
        }

        json_array_append_new(json_board, row);
    }
    json_object_set_new(msg, "board", json_board);
    
    // Altri campi
    json_object_set_new(msg, "turn", json_string(found_game->turn));
    json_object_set_new(msg, "state", json_string(game_state_to_string(found_game->state)));
    
    // Gestione winner
    if (strlen(found_game->winner) > 0) {
        json_object_set_new(msg, "winner", json_string(found_game->winner));
    } else {
        json_object_set_new(msg, "winner", json_null());
    }
    
    pthread_mutex_unlock(&server->games_mutex);
    return msg;
}

/**
 * Ritorna un json con tutte le partite create da un giocatore, altrimenti NULL
 */
json_t* list_games(server_t* server, const char* username){
    json_t* games = json_array();
    pthread_mutex_lock(&server->games_mutex);

    game_node_t* current = game_list->head;

    while(current){
        if(strcmp(current->game.player1,username) != 0){
            pthread_mutex_unlock(&server->games_mutex);
            
            json_t* game = create_json(server,current->game.id);
            json_array_append_new(games,game);
        }

        current = current->next;
    }

    pthread_mutex_unlock(&server->games_mutex);

    if(!games){
        return NULL;
    }

    return games;
}

/**
 * Gestione abbandono partita. Notifica il player in gioco che ha l'avversario ha abbandonato e dunque ha vinto la partita.
 * Ritorna 0 se lo stato della partita e l'invio della notifica vanno a buon fine, -1 se la partita non è in corso, -2 errore invio notifica
 */
short quit(server_t* server, game_t* game, const char* username){
    pthread_mutex_lock(&server->games_mutex);

    if(game->state != GAME_ONGOING){
        printf("[Errore - game.quit] La partita non è in corso\n");

        pthread_mutex_unlock(&server->games_mutex);
        return -1; // Gioco non in corso
    }

    // Imposta lo stato della partita a GAME_OVER e assegna il vincitore 
    game->state = GAME_OVER;
    if(strcmp(game->player1,username) == 0){
        strncpy(game->winner, game->player2, 63); // Imposta il vincitore
    } else {
        strncpy(game->winner, game->player1, 63); // Imposta il vincitore
    }


    pthread_mutex_unlock(&server->games_mutex);

    if(!send_to_player(server, create_request("quit", "L'avversario ha abbandonato", create_json(server, game->id)), game->winner)){

        // Nel caso di errore dell'invio reimposta lo stato della partita
        pthread_mutex_lock(&server->games_mutex);

        game->state = GAME_ONGOING;
        game->winner[0] = '\0';

        pthread_mutex_unlock(&server->games_mutex);
        return -2;
    }

    return 0;
}

/**
 * Gestione del rematch. Ritorna:
 * - 0 c'è un vincitore e vuole rigiocare
 * - 1 pareggio ed entrambi vogliono giocare
 * - 2 se si sta aspettando la risposta dall'avversario
 */
short rematch(server_t* server, game_t* game, const char* username){
    pthread_mutex_lock(&server->games_mutex);

    bool is_player1 = strcmp(game->player1, username) == 0;
    bool is_player2 = strcmp(game->player2, username) == 0;

    if(is_player1){
        game->rematch |= 1;
    }else if (is_player2){
        game->rematch |= 2;
    }

    if(strlen(game->winner) == 0){ //Terminata in pareggio
        if(game->rematch == 3){
            pthread_mutex_unlock(&server->games_mutex);
            reset_game(server,game,3);
            return 1;
        }
        
        pthread_mutex_unlock(&server->games_mutex);

        pthread_t tid;
        timeout_args_t *args = malloc(sizeof(timeout_args_t));
        args->game = game;
        args->server = server;
        strncpy(args->username, username, sizeof(args->username));

        pthread_create(&tid, NULL, rematch_timeout, args);
        pthread_detach(tid);
        return 2; //In attesa della risposta dell'altro player
    }

    pthread_mutex_unlock(&server->games_mutex);
    reset_game(server,game,game->rematch);
    return 0;
}