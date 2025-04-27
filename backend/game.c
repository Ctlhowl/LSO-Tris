#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <jansson.h>
#include <pthread.h>
#include "game.h"
#include "messages.h"  // Per send_game_update
#include "client.h"    // Per find_client_by_username

#define MAX_GAMES 50

static game_t games[MAX_GAMES];
static pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

// Inizializza una nuova partita
static void init_game(game_t* game, const char* player1) {
    memset(game, 0, sizeof(game_t));
    strncpy(game->player1, player1, sizeof(game->player1)-1);
    game->state = GAME_WAITING;
    
    // Inizializza la board vuota
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            game->board[i][j] = ' ';
        }
    }
}

// Cerca una partita disponibile o creane una nuova
int create_game(const char* username) {
    pthread_mutex_lock(&games_mutex);
    
    int available_slot = -1;
    int waiting_game = -1;
    
    for(int i = 0; i < MAX_GAMES; i++) {
        if(games[i].state == GAME_WAITING && strcmp(games[i].player1, username) != 0) {
            waiting_game = i;
            break;
        }
        if(games[i].state == GAME_OVER && available_slot == -1) {
            available_slot = i;
        }
    }
    
    if(waiting_game != -1) {
        // Unisciti a una partita in attesa
        strncpy(games[waiting_game].player2, username, sizeof(games[waiting_game].player2)-1);
        games[waiting_game].state = GAME_ONGOING;
        strncpy(games[waiting_game].turn, games[waiting_game].player1, sizeof(games[waiting_game].turn)-1);
        pthread_mutex_unlock(&games_mutex);
        send_game_update(waiting_game);
        return waiting_game;
    }
    
    if(available_slot == -1) {
        for(int i = 0; i < MAX_GAMES; i++) {
            if(games[i].state == GAME_OVER) {
                available_slot = i;
                break;
            }
        }
    }
    
    if(available_slot == -1) {
        pthread_mutex_unlock(&games_mutex);
        return -1; // Nessuno slot disponibile
    }
    
    // Crea una nuova partita
    init_game(&games[available_slot], username);
    pthread_mutex_unlock(&games_mutex);
    return available_slot;
}

// Controlla se c'è un vincitore
static char check_winner(game_t* game) {
    // Controlla le righe
    for(int i = 0; i < 3; i++) {
        if(game->board[i][0] != ' ' && 
           game->board[i][0] == game->board[i][1] && 
           game->board[i][1] == game->board[i][2]) {
            return game->board[i][0];
        }
    }
    
    // Controlla le colonne
    for(int j = 0; j < 3; j++) {
        if(game->board[0][j] != ' ' && 
           game->board[0][j] == game->board[1][j] && 
           game->board[1][j] == game->board[2][j]) {
            return game->board[0][j];
        }
    }
    
    // Controlla le diagonali
    if(game->board[0][0] != ' ' && 
       game->board[0][0] == game->board[1][1] && 
       game->board[1][1] == game->board[2][2]) {
        return game->board[0][0];
    }
    
    if(game->board[0][2] != ' ' && 
       game->board[0][2] == game->board[1][1] && 
       game->board[1][1] == game->board[2][0]) {
        return game->board[0][2];
    }
    
    return ' '; // Nessun vincitore
}

// Controlla se la partita è finita in pareggio
static int is_draw(game_t* game) {
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            if(game->board[i][j] == ' ') {
                return 1;
            }
        }
    }
    return 0;
}

// Esegue una mossa
int make_move(int game_id, const char* username, int x, int y) {
    if(game_id < 0 || game_id >= MAX_GAMES || x < 0 || x >= 3 || y < 0 || y >= 3) {
        return -1; // Parametri non validi
    }
    
    pthread_mutex_lock(&games_mutex);
    game_t* game = &games[game_id];
    
    if(game->state != GAME_ONGOING) {
        pthread_mutex_unlock(&games_mutex);
        return -2; // Partita non in corso
    }
    
    if(strcmp(game->turn, username) != 0) {
        pthread_mutex_unlock(&games_mutex);
        return -3; // Non è il turno del giocatore
    }
    
    if(game->board[x][y] != ' ') {
        pthread_mutex_unlock(&games_mutex);
        return -4; // Cella già occupata
    }
    
    // Esegui la mossa
    game->board[x][y] = (strcmp(game->player1, username) == 0) ? 'X' : 'O';
    
    // Controlla se la partita è finita
    char winner = check_winner(game);
    if(winner != ' ') {
        game->state = GAME_OVER;
        strncpy(game->winner, (winner == 'X') ? game->player1 : game->player2, 
               sizeof(game->winner)-1);
    } else if(is_draw(game)) {
        game->state = GAME_OVER;
        game->winner[0] = '\0'; // Pareggio
    } else {
        // Cambia turno
        strncpy(game->turn, (strcmp(game->player1, username) == 0) ? game->player2 : game->player1, 
               sizeof(game->turn)-1);
    }
    
    pthread_mutex_unlock(&games_mutex);
    send_game_update(game_id);
    return 0;
}

// Gestisce la richiesta di rematch
int handle_rematch(int game_id, const char* player, int choice) {
    if(game_id < 0 || game_id >= MAX_GAMES) {
        return -1;
    }
    
    pthread_mutex_lock(&games_mutex);
    game_t* game = &games[game_id];
    
    if(game->state != GAME_OVER) {
        pthread_mutex_unlock(&games_mutex);
        return -2; // Partita non finita
    }
    
    // Aggiorna la scelta del rematch
    if(strcmp(game->player1, player) == 0) {
        game->rematch = (game->rematch & ~1) | (choice ? 1 : 0);
    } else if(strcmp(game->player2, player) == 0) {
        game->rematch = (game->rematch & ~2) | (choice ? 2 : 0);
    } else {
        pthread_mutex_unlock(&games_mutex);
        return -3; // Giocatore non trovato
    }
    
    // Se entrambi vogliono il rematch, ricomincia la partita
    if(game->rematch == 3) {
        char old_player2[64];
        strncpy(old_player2, game->player2, sizeof(old_player2));

        init_game(game, game->player1);

        strncpy(game->player2, old_player2, sizeof(game->player2));
        game->state = GAME_ONGOING;
        strncpy(game->turn, game->player1, sizeof(game->turn));
        game->rematch = 0;
    }
    
    pthread_mutex_unlock(&games_mutex);
    send_game_update(game_id);
    return 0;
}

// Invia l'aggiornamento della partita a entrambi i giocatori
void send_game_update(int game_id) {
    if(game_id < 0 || game_id >= MAX_GAMES) return;
    
    pthread_mutex_lock(&games_mutex);
    game_t* game = &games[game_id];
    
    // Crea il messaggio JSON
    json_t* msg = json_object();
    json_object_set_new(msg, "event", json_string("game_update"));
    json_object_set_new(msg, "game_id", json_integer(game_id));
    json_object_set_new(msg, "state", json_integer(game->state));
    json_object_set_new(msg, "player1", json_string(game->player1));
    json_object_set_new(msg, "player2", json_string(game->player2));
    json_object_set_new(msg, "turn", json_string(game->turn));
    
    // Aggiungi la board
    json_t* board_json = json_array();
    for(int i = 0; i < 3; i++) {
        json_t* row = json_array();
        for(int j = 0; j < 3; j++) {
            char cell[2] = {game->board[i][j], '\0'};
            json_array_append_new(row, json_string(cell));
        }
        json_array_append_new(board_json, row);
    }
    json_object_set_new(msg, "board", board_json);
    
    // Aggiungi il vincitore se presente
    if(game->state == GAME_OVER && game->winner[0] != '\0') {
        json_object_set_new(msg, "winner", json_string(game->winner));
    }
    
    // Aggiungi lo stato del rematch
    json_object_set_new(msg, "rematch", json_integer(game->rematch));
    
    // Invia il messaggio a entrambi i giocatori
    int sock1 = find_client_by_username(game->player1);
    int sock2 = find_client_by_username(game->player2);
    
    if(sock1 != -1) {
        char* msg_str = json_dumps(msg, JSON_COMPACT);
        send(sock1, msg_str, strlen(msg_str), 0);
        free(msg_str);
    }
    
    if(sock2 != -1) {
        char* msg_str = json_dumps(msg, JSON_COMPACT);
        send(sock2, msg_str, strlen(msg_str), 0);
        free(msg_str);
    }
    
    json_decref(msg);
    pthread_mutex_unlock(&games_mutex);
}