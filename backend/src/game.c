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
game_t* new_node(const char* player1){
    game_t* new_game = (game_t*)malloc(sizeof(game_t));
    if(!new_game){
        perror("Failed to allocate memory for new game");
        return NULL;
    }

    new_game->id = game_list->count;
    strncpy(new_game->player1, player1, sizeof(new_game->player1) - 1);
    new_game->player2[0] = '\0';

    memset(new_game->board, 0, sizeof(new_game->board));
    
    strncpy(new_game->turn, player1, sizeof(new_game->turn) - 1);
    new_game->state = GAME_WAITING;
    new_game->winner[0] = '\0' ; 
    new_game->rematch = -1;

    return new_game;
}

bool game_add(game_t* game){
    game_node_t* new_node = (game_node_t*)malloc(sizeof(game_node_t));
    if (!new_node) {
        perror("Failed to allocate memory for new game node");
        return false;
    }
    
    new_node->game = *game;
    new_node->next = game_list->head;
    game_list->head = new_node;
    game_list->count++;
            
    return true;
}

bool game_remove(const size_t id) {    
    game_node_t** pp = &game_list->head;
    game_node_t* current = game_list->head;
    
    while (current) {
        if (current->game.id == id) {
            *pp = current->next;
            free(current);

            game_list->count--;

            return true;
        }
        pp = &current->next;
        current = current->next;
    }
    
    return false;
}

const char* game_state_to_string(game_state_t state) {
    switch (state) {
        case GAME_WAITING: return "GAME_WAITING";
        case GAME_ONGOING: return "GAME_ONGOING";
        case GAME_OVER:    return "GAME_OVER";
        default:           return "unknown";
    }
}

bool is_opponent_available(const char* player2){
    game_node_t *curr = game_list->head;
    
    size_t index = 0;
    while(curr && index <= game_list->count){
        game_t game = curr->game;
        if (game.state == GAME_ONGOING){
            if(strcmp(game.player1, player2) == 0 || strcmp(game.player2, player2) == 0){
                printf("%s è gia impegnato in un'altra partita\n", player2);
                return false;
            }
        }
    }

    return true;
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
            perror("Failed to allocate memory for game list");
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
        return -1;
    }

    // Creazione del nodo e allocazione della memoria
    game_t* new_game = new_node(player1);
    if(!new_game){
        pthread_mutex_unlock(&server->games_mutex);
        return -1;
    }

    if(!game_add(new_game)){
        pthread_mutex_unlock(&server->games_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&server->games_mutex);
    return new_game->id;
}

short request_join_game(server_t* server, size_t game_id, const char *player2) {
    if (game_id >= game_list->count) {
        return -1;  // Gioco non valido
    }

    pthread_mutex_lock(&server->games_mutex);
    game_node_t *curr = game_list->head;
    
    size_t index = 0;
    while(curr && index <= curr->game.id){
        if(curr->game.id == game_id){
            // Verifica che la partita sia in stato di "attesa"
            if (curr->game.state != GAME_WAITING) {
                pthread_mutex_unlock(&server->games_mutex);
                return -2;  // La partita non è in attesa, non si può unire
            }

            // Invia la richiesta di join al creatore della partita (player1)
            json_t *game_param = json_object();
            json_object_set_new(game_param, "game_id", json_integer(game_id));
            json_object_set_new(game_param, "player2", json_string(player2));

            json_t *join_request_msg = create_request("join_request", "", game_param);

             // Invia il messaggio al creatore (player1)
             send_to_player(server, join_request_msg, curr->game.player1);

             pthread_mutex_unlock(&server->games_mutex);
             return 0;  // Successo
        }

        curr = curr->next;
    }

    pthread_mutex_unlock(&server->games_mutex);
    return -1;  // Partita non trovata
   
}


short accept_join_request(server_t* server, size_t game_id, const char *player2){
    if (game_id >= game_list->count) {
        return -1;  // Gioco non valido
    }

    pthread_mutex_lock(&server->games_mutex);
    game_node_t *curr = game_list->head;
    
    size_t index = 0;
    while(curr && index <= curr->game.id){
        game_t game = curr->game;
        if(game.id == game_id){
            
            // Verifica che la partita sia in stato di "attesa"
            if (game.state != GAME_WAITING) {
                pthread_mutex_unlock(&server->games_mutex);
                return -2;  // La partita non è in attesa, non si può unire
            }


            // Verifico se l'avversario é impegnato in un'altra partita
            if(!is_opponent_available(player2)){
                pthread_mutex_unlock(&server->games_mutex);
                return -4;  // L'avversario é giá impegnato 
            }

            // Aggiungi il secondo giocatore alla partita
            strncpy(game.player2, player2, sizeof(game.player2) - 1);
            game.state = GAME_ONGOING;

            pthread_mutex_unlock(&server->games_mutex);

            // Notifica a entrambi i giocatori che la partita è iniziata
            //send_game_update(game_id);

            return 0;  // Successo
        }

        curr = curr->next;
    }

    pthread_mutex_unlock(&server->games_mutex);
    return -1;  // Partita non trovata
}


/**
 * Serializza la struttura game_t
 */
json_t* create_json(server_t* server, size_t id){
    json_t* msg = json_object();
    if (!msg) return NULL;
    
    pthread_mutex_lock(&server->games_mutex);
    
    size_t index = 0;
    game_node_t* current = game_list->head;
    while (current && index <= id) {
        if (current->game.id == id) {
            game_t game =  current->game;

            json_object_set_new(msg, "id", json_integer(game.id));
            json_object_set_new(msg, "player1", json_string(game.player1));

            strlen(game.player2) > 0 ? 
                json_object_set_new(msg, "player2", json_string(game.player2)) : 
                json_object_set_new(msg, "player2", json_null());

            // serializzazione board nel formato
            /* 
                [
                    ["X", "O", " "],
                    [" ", "X", "O"],
                    ["O", " ", "X"]
                ]
            */
            json_t *json_board = json_array();
            for (int i = 0; i < 3; i++) {
                json_t *row = json_array();
                for (int j = 0; j < 3; j++) {
                    char cell[2] = { game.board[i][j], '\0' }; 
                    json_array_append_new(row, json_string(cell));
                }
                json_array_append_new(json_board, row);
            }
            json_object_set_new(msg, "board", json_board);
                

            json_object_set_new(msg, "turn", json_string(game.turn));
            json_object_set_new(msg, "state", json_string(game_state_to_string(game.state)));

            strlen(game.winner) > 0 ? 
                json_object_set_new(msg, "winner", json_string(game.player2)) : 
                json_object_set_new(msg, "winner", json_null());
        }
        
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->games_mutex);
    return msg;
}