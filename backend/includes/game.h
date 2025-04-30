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
    char player1[64];   // First player name
    char player2[64];   // Second player name
    char board[3][3];
    char turn[64];      // Active player's turn
    game_state_t state;
    char winner[64];    // Winner of the game, if empty and the status is finished then it is a draw
    unsigned short int rematch;     // 1 = player 1 wants a rematch, 2 = player 2 wants a rematch, 3 = both players want a rematch, 0 otherwise
} game_t;

typedef struct game_node {
    game_t game;
    struct game_node* next;
} game_node_t;

typedef struct {
    game_node_t* head;
    size_t count;
} game_list_t;

extern game_list_t* game_list;

void game_init(server_t* server);
void game_cleanup(server_t* server);

ssize_t create_game(server_t* server, const char* player1);
short request_join_game(server_t* server, size_t game_id, const char *player2);
short accept_join_request(server_t* server, size_t game_id, const char *player2);

json_t* create_json(server_t* server, size_t id);

#endif