#ifndef GAME_H
#define GAME_H

typedef enum {
    GAME_WAITING,
    GAME_ONGOING,
    GAME_OVER
} game_state_t;

typedef struct {
    char player1[64];   // First player name
    char player2[64];   // Second player name
    char board[3][3];
    char turn[64];      // Active player's turn
    game_state_t state;
    char winner[64];    // Winner of the game, if empty and the status is finished then it is a draw
    unsigned short int rematch;     // 1 = player 1 wants a rematch, 2 = player 2 wants a rematch, 3 = both players want a rematch, 0 otherwise
} game_t;


// Funzioni per la gestione delle partite
int create_game(const char* username);
void send_game_update(int game_id);
int make_move(int game_id, const char* username, int x, int y);
int handle_rematch(int game_id, const char* player, int choice);
#endif