#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <jansson.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 50
#define MAX_GAMES 50

typedef struct {
    int sock;
    char username[64];
} client_t;

typedef enum {      
    GAME_WAITING,    // Gioco in attesa del secondo giocatore
    GAME_ONGOING,    // Gioco in corso
    GAME_OVER        // Gioco terminato
} game_state_t;

typedef struct {
    char player1[64];  // Nome del primo giocatore
    char player2[64];  // Nome del secondo giocatore
    char board[3][3];  // Tavola di gioco 3x3
    char turn[64];     // Giocatore il cui turno è attivo
    game_state_t state; // Stato del gioco (in attesa, in corso, terminato)
    char winner[64];       // Vincitore della partita,se vuoto e lo stato è terminato allora è un pareggio
    int rematch_player1; // Esprime la volontà del primo giocatore di continuare a giocare
    int rematch_player2; // Esprime la volontà del secondo giocatore di continuare a giocare
} game_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;

game_t games[MAX_GAMES];
int game_count = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

//GESTIONE MESSAGGI BROADCAST

void broadcast_json(json_t *msg, int exclude_fd) {
    char *msg_str = json_dumps(msg, JSON_COMPACT);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].sock != exclude_fd) {
            send(clients[i].sock, msg_str, strlen(msg_str), 0);
            send(clients[i].sock, "\n", 1, 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(msg_str);
}

//GESTIONE PARTITE

int create_game(const char *username) {
    pthread_mutex_lock(&games_mutex);  // Blocco del mutex per proteggere l'accesso alla lista dei giochi

    // Verifica se ci sono slot liberi per nuove partite
    if (game_count >= MAX_GAMES) {
        pthread_mutex_unlock(&games_mutex);  // Sblocca il mutex prima di restituire
        return -1;  // Massimo numero di giochi raggiunto
    }

    // Creazione della partita
    game_t *g = &games[game_count++];  // Ottieni un riferimento alla partita
    strncpy(g->player1, username, sizeof(g->player1) - 1);  // Assegna il creatore della partita
    memset(g->board, 0, sizeof(g->board));  // Inizializza la tavola vuota
    g->state = GAME_WAITING;   // Stato iniziale (attesa di un secondo giocatore)
    strncpy(g->turn, username, sizeof(g->turn) - 1);  // Il primo giocatore inizia
    g->player2[0] = '\0';  // Il secondo giocatore non è ancora assegnato
    g->winner[0] = '\0' ;  // Non essendo ancora iniziata non c'è un vincitore
    g->rematch_player1 = -1; // Non essendo ancora finita non si può esprimere la volontà di continuare a giocare
    g->rematch_player2 = -1;

    pthread_mutex_unlock(&games_mutex);  // Sblocca il mutex dopo aver creato la partita

    return game_count - 1;  // Restituisce l'ID della partita creata
}

void send_to_player(const char *player, json_t *message) {
    // Converte il messaggio JSON in stringa
    char *msg_str = json_dumps(message, JSON_COMPACT);

    // Acquisisci il mutex per proteggere l'accesso all'array clients
    pthread_mutex_lock(&clients_mutex);

    // Cerca il socket del giocatore
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].username, player) == 0) {
            send(clients[i].sock, msg_str, strlen(msg_str), 0);
            send(clients[i].sock, "\n", 1, 0);  // Aggiunge un terminatore di riga (facoltativo)
            printf("Messaggio inviato a %s: %s\n", player, msg_str);
            break;
        }
    }

    // Rilascia il mutex dopo aver completato l'accesso all'array clients
    pthread_mutex_unlock(&clients_mutex);

    // Libera la memoria del messaggio
    free(msg_str);
}

void send_game_update(int game_id) {
    pthread_mutex_lock(&games_mutex);
    game_t *game = &games[game_id];
    json_t *notif = json_object();
    
    if (game->state == GAME_ONGOING) {
        json_object_set_new(notif, "event", json_string("game_update"));
        json_object_set_new(notif, "game_id", json_integer(game_id));
        json_object_set_new(notif, "turn", json_string(game->turn));
    } else if (game->state == GAME_OVER) {
        json_object_set_new(notif, "event", json_string("game_over"));
        json_object_set_new(notif, "game_id", json_integer(game_id));
        if (game->winner[0] != '\0') {
            json_object_set_new(notif, "winner", json_string(game->winner));
        } else {
            json_object_set_new(notif, "result", json_string("draw"));
        }
    }

    // Invia il messaggio a entrambi i giocatori
    send_to_player(game->player1, notif);
    send_to_player(game->player2, notif);
    pthread_mutex_unlock(&games_mutex);

    json_decref(notif);
}

int check_drawOrWin(int game_id){
    pthread_mutex_lock(&games_mutex);
    game_t *game = &games[game_id];

    if(game->state != GAME_OVER){
        pthread_mutex_unlock(&games_mutex);
        return -1; //Partita ancora non terminata
    }

    if(strlen(game->winner) > 0){
        pthread_mutex_unlock(&games_mutex);
        return 1; //C'è un vincitore
    }

    pthread_mutex_unlock(&games_mutex);
    return 0; //Pareggio
}

int find_sock_by_username(const char *player){
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < client_count; i++){
        if(strcmp(clients[i].username,player) == 0){
            return clients[i].sock;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return -1;
}

void reset_game_for_rematch(game_t *g, const char *new_owner, int game_id) {
    pthread_mutex_lock(&games_mutex);
    memset(g->board, 0, sizeof(g->board));
    strncpy(g->player1, new_owner, sizeof(g->player1) - 1);
    g->player2[0] = '\0';
    strncpy(g->turn, new_owner, sizeof(g->turn) - 1);
    g->state = GAME_WAITING;
    g->winner[0] = '\0';
    g->rematch_player1 = -1;
    g->rematch_player2 = -1;
    pthread_mutex_unlock(&games_mutex);

    json_t *response = json_object();
    json_t *event = json_object();
    json_object_set_new(response, "response", json_string("game_resetted"));
    json_object_set_new(response, "game_id", json_integer(game_id));
    json_object_set_new(event, "event", json_string("game waiting for players"));
    json_object_set_new(event, "game_id", json_integer(game_id));
    json_object_set_new(event, "created_by", json_string(new_owner));
    int client_sock = find_sock_by_username(new_owner);

    if(client_sock != -1){
        broadcast_json(event,client_sock);
    }
}


void handle_rematch(int game_id, const char *player, int choise){
    if(game_id < 0 || game_id >= game_count){
        return; //Gioco non valido
    }

    int outcome = check_drawOrWin(game_id);
    json_t *notif = json_object();
    pthread_mutex_lock(&games_mutex);
    game_t *game = &games[game_id];
    
    
    if(strcmp(game->player1,player) != 0 && strcmp(game->player2,player) != 0){
        pthread_mutex_unlock(&games_mutex);
        json_object_set_new(notif, "error", json_string("Player not part of the game"));
        send_to_player(player,notif);
        json_decref(notif);
        return;
    }

    if(outcome == 1){
        if(strcmp(game->winner,player) != 0){
            pthread_mutex_unlock(&games_mutex);
            json_object_set_new(notif, "error", json_string("Only the winner can request rematch"));
            send_to_player(player,notif);
            json_decref(notif);
            return;
        }

        if(choise == 1){
            pthread_mutex_unlock(&games_mutex);
            reset_game_for_rematch(game,player,game_id);
            return;
        }
    }

    if(outcome == 0){
        if(strcmp(game->player1,player) == 0){
            game->rematch_player1 = choise;
        }
        if(strcmp(game->player2,player) == 0){
            game->rematch_player2 = choise;
        }

        if(game->rematch_player1 && game->rematch_player2){
            pthread_mutex_unlock(&games_mutex);
            int new_game_id = create_game(game->player1);

            pthread_mutex_lock(&games_mutex);

            game_t *new_game = &games[new_game_id];
            strncpy(new_game->player2, game->player2, sizeof(new_game->player2) - 1);
            new_game->state = GAME_ONGOING;
            json_object_set_new(notif, "event", json_string("rematch_started"));
            send_to_player(game->player1,notif);
            send_to_player(game->player2,notif);

            pthread_mutex_unlock(&games_mutex);
            send_game_update(new_game_id);
            json_decref(notif);
            return;
        }

        if(game->rematch_player1){
            pthread_mutex_unlock(&games_mutex);
            reset_game_for_rematch(game,game->player1,game_id);
            return;
        }
        if(game->rematch_player2){
            pthread_mutex_unlock(&games_mutex);
            reset_game_for_rematch(game,game->player2,game_id);
            return;
        }
    }

    pthread_mutex_unlock(&games_mutex);
    json_decref(notif);
    return;
}

int quit(int game_id, const char *player){
    if (game_id < 0 || game_id >= game_count) {
        return -1;  // Gioco non valido
    }

    pthread_mutex_lock(&games_mutex);
    game_t *game = &games[game_id];
   

    if(game->state != GAME_ONGOING){
        pthread_mutex_unlock(&games_mutex);
        printf("Non si può quittare una partita che non è in corso\n");
        return -2; // Gioco non in corso
    }

    if(strcmp(game->player1,player) != 0 && strcmp(game->player2,player) != 0){
        pthread_mutex_unlock(&games_mutex);
        return -3; //Richiesta di quit da parte di un player non in partita
    }

    game->state = GAME_OVER;
    json_t *notifWinner = json_object();
    json_t *notifLoser = json_object();

    //json per il vincitore
    json_object_set_new(notifWinner, "event", json_string("game_over"));
    json_object_set_new(notifWinner, "game_id", json_integer(game_id));
    json_object_set_new(notifWinner, "reason", json_string("opponent quit"));

    //json per il player che ha quittato
    json_object_set_new(notifLoser, "event", json_string("successfull_quit"));
    json_object_set_new(notifLoser, "game_id", json_integer(game_id));

    if(strcmp(player,game->player1) == 0){
        strncpy(game->winner, game->player2, 63);// Imposta il vincitore
        json_object_set_new(notifWinner, "winner", json_string(game->winner));
        send_to_player(game->player2,notifWinner);
        send_to_player(game->player1,notifLoser);
    }else {
        strncpy(game->winner, game->player1, 63);// Imposta il vincitore
        json_object_set_new(notifWinner, "winner", json_string(game->winner));
        send_to_player(game->player1,notifWinner);
        send_to_player(game->player2,notifLoser);
    }

    pthread_mutex_unlock(&games_mutex);
    json_decref(notifWinner);
    json_decref(notifLoser);

    return 0;
    
}

int check_win(game_t *game) {
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
    int filled = 1;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (game->board[i][j] == '\0') {
                filled = 0;
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

int make_move(int game_id, const char *username, int x, int y) {
    if (game_id < 0 || game_id >= game_count) {
        return -1;  // Gioco non valido
    }

    pthread_mutex_lock(&games_mutex);
    game_t *game = &games[game_id];
    
    if(game->state != GAME_ONGOING){
        pthread_mutex_unlock(&games_mutex);
        return -2; 
    }

    // Verifica che sia il turno del giocatore
    if (strcmp(game->turn, username) != 0) {
        pthread_mutex_unlock(&games_mutex);
        return -3;  // Non è il turno del giocatore
    }

    // Verifica che la posizione della mossa sia valida
    if (x < 0 || x > 2 || y < 0 || y > 2 || game->board[x][y] != '\0') {
        pthread_mutex_unlock(&games_mutex);
        return -4;  // Posizione non valida
    }

    // Esegui la mossa
    char symbol = (strcmp(game->turn, game->player1) == 0) ? 'X' : 'O';
    game->board[x][y] = symbol;

    // Controlla se c'è un vincitore
    if (check_win(game) == 1) {
        game->state = GAME_OVER;
        strncpy(game->winner, username, 63);// Imposta il vincitore
        pthread_mutex_unlock(&games_mutex);
        return 1;  // Vincitore trovato
    }

    // Cambia il turno
    strncpy(game->turn, (strcmp(game->turn, game->player1) == 0) ? game->player2 : game->player1, 63);
    pthread_mutex_unlock(&games_mutex);
    return 0;  // Mossa successiva, gioco continua
}

int request_join_game(int game_id, const char *player2) {
    if (game_id < 0 || game_id >= game_count) {
        return -1;  // Gioco non valido
    }

    pthread_mutex_lock(&games_mutex);
    game_t *game = &games[game_id];

    // Verifica che la partita sia in stato di "attesa"
    if (game->state != GAME_WAITING) {
        pthread_mutex_unlock(&games_mutex);
        return -2;  // La partita non è in attesa, non si può unire
    }

    // Verifica che il secondo giocatore non sia già nella partita
    if (strcmp(game->player1, player2) == 0 || strcmp(game->player2, player2) == 0) {
        pthread_mutex_unlock(&games_mutex);
        return -3;  // Giocatore già nella partita
    }

    // Invia la richiesta di join al creatore della partita (player1)
    json_t *join_request_msg = json_object();
    json_object_set_new(join_request_msg, "event", json_string("join_request"));
    json_object_set_new(join_request_msg, "game_id", json_integer(game_id));
    json_object_set_new(join_request_msg, "player2", json_string(player2));
    
    // Invia il messaggio al creatore (player1)
    send_to_player(game->player1, join_request_msg);
    json_decref(join_request_msg);
    pthread_mutex_unlock(&games_mutex);
    return 0;  // Successo
}

int accept_join_request(int game_id, const char *player2) {
    if (game_id < 0 || game_id >= game_count) {
        return -1;  // Gioco non valido
    }

    pthread_mutex_lock(&games_mutex);

    game_t *game = &games[game_id];

    if(game->state != GAME_WAITING){
        pthread_mutex_unlock(&games_mutex);
        return -2;
    }

    // Verifico se il creatore è impegnato in un'altra partita
    for(int i = 0; i < game_count; i++){
        if(i != game_id && games[i].state == GAME_ONGOING){
            if(strcmp(games[i].player1,game->player1) == 0 || strcmp(games[i].player2,game->player1) == 0){
                printf("Non può accettare la richiesta poichè può giocare una partita alla volta\n");
                pthread_mutex_unlock(&games_mutex);
                return -3; // Non può accettare la richiesta poichè può giocare una partita alla volta
            }
            if(strcmp(games[i].player1,player2) == 0 || strcmp(games[i].player2,player2) == 0){
                printf("%s è gia impegnato in un'altra partita",player2);
                pthread_mutex_unlock(&games_mutex);
                return -4;
            }
        }
    }

    // Aggiungi il secondo giocatore alla partita
    strncpy(game->player2, player2, sizeof(game->player2) - 1);
    game->state = GAME_ONGOING;  // Cambia lo stato della partita a "In corso"
    strncpy(game->turn, game->player1, sizeof(game->turn) - 1);  // Inizia il turno con il primo giocatore

    pthread_mutex_unlock(&games_mutex);

    // Notifica a entrambi i giocatori che la partita è iniziata
    send_game_update(game_id);

    return 0;  // Successo
}

const char* enum_to_string(game_state_t state){
    if(state == GAME_WAITING){
        return "IN ATTESA";
    }
    if(state == GAME_ONGOING){
        return "IN GIOCO";
    }
    
    return "TERMINATA";
}

void remove_games(const char *player){
    pthread_mutex_lock(&games_mutex);

    for (int i = 0; i < game_count;) {
        game_t *game = &games[i];

        // Rimuove le partite dove l'utente è il creatore
        if (strcmp(game->player1, player) == 0 && game->state == GAME_WAITING) {
            // Sovrascrive questa partita con l'ultima
            games[i] = games[game_count - 1];
            game_count--;
            continue;  // Ri-controlla la stessa posizione ora che c'è un nuovo gioco lì
        }

        i++;
    }

    pthread_mutex_unlock(&games_mutex);
}

void notify_disconnection(const char *player){
    json_t *notifWinner = json_object();
    json_object_set_new(notifWinner, "event", json_string("game_over"));
    json_object_set_new(notifWinner, "reason", json_string("opponent disconnected"));

    pthread_mutex_lock(&games_mutex);

    for (int i = 0; i < game_count; i++) {
        game_t *game = &games[i];

        if(game->state == GAME_ONGOING){
            if(strcmp(game->player1, player) == 0 ){
                json_object_set_new(notifWinner, "game_id", json_integer(i));
                strncpy(game->winner, game->player2, 63);// Imposta il vincitore
                json_object_set_new(notifWinner, "winner", json_string(game->winner));
                send_to_player(game->player2,notifWinner);
            }

            if(strcmp(game->player2, player) == 0){
                json_object_set_new(notifWinner, "game_id", json_integer(i));
                strncpy(game->winner, game->player1, 63);// Imposta il vincitore
                json_object_set_new(notifWinner, "winner", json_string(game->winner));
                send_to_player(game->player1,notifWinner);
            }
        }

        game->state = GAME_OVER;
    }

    json_decref(notifWinner);
    pthread_mutex_unlock(&games_mutex);
}

//GESTIONE CLIENT

void remove_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].sock == sock) {
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int is_username_unique(const char *username) {
    int unique = 1;

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].username, username) == 0) {
            unique = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    return unique;
}


//ROUTING  

void route_request(int client_sock, const char *username, const char *json_str) {
    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);
    if (!root) return;

    json_t *method = json_object_get(root, "method");
    if (!json_is_string(method)) {
        json_decref(root);
        return;
    }

    const char *method_str = json_string_value(method);

    if (strcmp(method_str, "broadcast") == 0) {
        json_t *msg = json_object_get(root, "message");
        if (json_is_string(msg)) {
            json_t *event = json_object();
            json_object_set_new(event, "event", json_string("message"));
            json_object_set_new(event, "from", json_string(username));
            json_object_set(event, "message", msg);
            broadcast_json(event, client_sock);
            json_decref(event);
        }

    } else if (strcmp(method_str, "list_users") == 0) {
        pthread_mutex_lock(&clients_mutex);
        json_t *arr = json_array();
        for (int i = 0; i < client_count; ++i) {
            json_array_append_new(arr, json_string(clients[i].username));
        }
        pthread_mutex_unlock(&clients_mutex);

        json_t *response = json_object();
        json_object_set_new(response, "response", json_string("user_list"));
        json_object_set_new(response, "users", arr);

        char *msg_str = json_dumps(response, JSON_COMPACT);
        send(client_sock, msg_str, strlen(msg_str), 0);
        send(client_sock, "\n", 1, 0);
        free(msg_str);
        json_decref(response);

    }else if (strcmp(method_str, "create_game") == 0) {
        // Creazione della partita
        int game_id = create_game(username);
        if (game_id >= 0) {
            json_t *response = json_object();
            json_t *event = json_object();
            json_object_set_new(response, "response", json_string("game_created"));
            json_object_set_new(response, "game_id", json_integer(game_id));
            json_object_set_new(event, "event", json_string("game waiting for players"));
            json_object_set_new(event, "game_id", json_integer(game_id));
            json_object_set_new(event, "created_by", json_string(username));
            broadcast_json(event,client_sock);
            char *msg_str = json_dumps(response, JSON_COMPACT);
            send(client_sock, msg_str, strlen(msg_str), 0);
            send(client_sock, "\n", 1, 0);
            free(msg_str);
            json_decref(response);
            json_decref(event);
        }

    } else if (strcmp(method_str, "join_request") == 0) {
        int game_id = json_integer_value(json_object_get(root, "game_id"));
        if (game_id >= 0) {
            // La logica di join_request potrebbe inviare una richiesta al creatore
            int result = request_join_game(game_id, username);
            if (result == 0) {
                json_t *response = json_object();
                json_object_set_new(response, "response", json_string("join_requested"));
                json_object_set_new(response, "game_id", json_integer(game_id));
                char *msg_str = json_dumps(response, JSON_COMPACT);
                send(client_sock, msg_str, strlen(msg_str), 0);
                send(client_sock, "\n", 1, 0);
                free(msg_str);
                json_decref(response);
            }
        }
    }
    // Gestione del metodo 'accept_join'
    else if (strcmp(method_str, "accept_join") == 0) {
        int game_id = json_integer_value(json_object_get(root, "game_id"));
        const char* opponent = json_string_value(json_object_get(root, "player"));

        if (game_id >= 0) {
            // Il creatore accetta la richiesta di join
            int result = accept_join_request(game_id, opponent);
            if (result == 0) {
                json_t *response = json_object();
                json_t *event = json_object();
                json_object_set_new(response, "response", json_string("join_accepted"));
                json_object_set_new(response, "game_id", json_integer(game_id));
                json_object_set_new(event, "event", json_string("game is started"));
                json_object_set_new(event, "game_id", json_integer(game_id));
                json_object_set_new(event, "player1", json_string(username));
                json_object_set_new(event, "player2", json_string(opponent));
                broadcast_json(event,client_sock);
                char *msg_str = json_dumps(response, JSON_COMPACT);
                send(client_sock, msg_str, strlen(msg_str), 0);
                send(client_sock, "\n", 1, 0);
                free(msg_str);
                json_decref(response);
                json_decref(event);
            }
            if(result == -2){
                const char *err = "{\"error\":\"the match is no long available\"}\n";
                send(client_sock, err, strlen(err), 0);
            }
            if(result == -4){
                const char *err = "{\"error\":\"the opponent is busy in another game\"}\n";
                send(client_sock, err, strlen(err), 0);
            }
        }
    }
    else if (strcmp(method_str,"reject_join") == 0){
        int game_id = json_integer_value(json_object_get(root, "game_id"));
        const char* opponent = json_string_value(json_object_get(root, "player"));

        if(game_id >= 0){
            json_t *response = json_object();
            json_t *confirm = json_object();
            json_object_set_new(response, "response", json_string("join_rejected"));
            json_object_set_new(response, "game_id", json_integer(game_id));
            json_object_set_new(response, "creator", json_string(username));
            json_object_set_new(confirm, "event", json_string("join successfully rejected"));
            json_object_set_new(confirm, "game_id", json_integer(game_id));
            json_object_set_new(confirm, "player", json_string(opponent));

            send_to_player(username,confirm);
            send_to_player(opponent,response);
        }

    }
    else if (strcmp(method_str, "list_availableGames") == 0){
        pthread_mutex_lock(&games_mutex);
        json_t *arr = json_array();
        for (int i = 0; i < game_count; ++i) {
            if(games[i].state == GAME_WAITING && strcmp(games[i].player1,username) != 0){
                json_t *obj = json_object();

                json_object_set_new(obj, "game_id", json_integer(i));
                json_object_set_new(obj, "player1", json_string(games[i].player1));
                json_object_set_new(obj, "turn", json_string(games[i].turn));
                json_object_set_new(obj, "state", json_string("WAITING FOR OPPONENT"));
                json_object_set_new(obj, "winner",json_string("maybe you are the one"));

                json_array_append_new(arr, obj);
            } 
        }
        pthread_mutex_unlock(&games_mutex);

        json_t *response = json_object();
        json_object_set_new(response, "response", json_string("available_games"));
        json_object_set_new(response, "games", arr);

        char *msg_str = json_dumps(response, JSON_COMPACT);
        send(client_sock, msg_str, strlen(msg_str), 0);
        send(client_sock, "\n", 1, 0);
        free(msg_str);
        json_decref(response);
    }
    else if (strcmp(method_str, "list_userGames") == 0){
        pthread_mutex_lock(&games_mutex);
        json_t *arr = json_array();
        for (int i = 0; i < game_count; ++i) {
            if(strcmp(games[i].player1,username) == 0){
                json_t *obj = json_object();

                json_object_set_new(obj, "idGame", json_integer(i));
                json_object_set_new(obj, "player1", json_string(games[i].player1));
                json_object_set_new(obj, "player2", json_string(games[i].player2));
                json_object_set_new(obj, "turn", json_string(games[i].turn));
                json_object_set_new(obj, "state", json_string(enum_to_string(games[i].state)));
                json_object_set_new(obj, "winner",json_string(games[i].winner));

                json_array_append_new(arr, obj);
            } 
        }
        pthread_mutex_unlock(&games_mutex);

        json_t *response = json_object();
        json_object_set_new(response, "response", json_string("user_games"));
        json_object_set_new(response, "games", arr);

        char *msg_str = json_dumps(response, JSON_COMPACT);
        send(client_sock, msg_str, strlen(msg_str), 0);
        send(client_sock, "\n", 1, 0);
        free(msg_str);
        json_decref(response);
    }
    else if (strcmp(method_str, "game_move") == 0) {
        int game_id = json_integer_value(json_object_get(root, "game_id"));
        int x = json_integer_value(json_object_get(root, "x"));
        int y = json_integer_value(json_object_get(root, "y"));

        int result = make_move(game_id, username, x, y);
        
        if(result == -3){
            const char *err = "{\"error\":\"Not your turn\"}\n";
            send(client_sock, err, strlen(err), 0);
        }
        else if(result == -4){
            const char *err = "{\"error\":\"Invalid position\"}\n";
            send(client_sock, err, strlen(err), 0);
        }
        else{
            send_game_update(game_id);
        }
    }
    else if (strcmp(method_str, "game_quit") == 0){
        int game_id = json_integer_value(json_object_get(root, "game_id"));
        quit(game_id,username);
    }
    else if (strcmp(method_str, "rematch") == 0){
        int game_id = json_integer_value(json_object_get(root, "game_id"));
        int choise = json_is_true(json_object_get(root, "choise")) ? 1 : 0;

        handle_rematch(game_id,username,choise);
    }
    else {
        const char *err = "{\"error\":\"Unknown method\"}\n";
        send(client_sock, err, strlen(err), 0);
    }

    json_decref(root);
}


//HANDLER CLIENT

void *client_handler(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char buffer[1024];
    char username[64] = {0};

    // Invia richiesta di login
    const char *login_request = "{\"request\":\"login\",\"message\":\"Please send your username\"}\n";
    send(client_sock, login_request, strlen(login_request), 0);

    // Attendi login valido
    while (1) {
        int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            close(client_sock);
            return NULL;
        }

        buffer[bytes_received] = '\0';

        json_error_t error;
        json_t *root = json_loads(buffer, 0, &error);
        if (!root) continue;

        json_t *method = json_object_get(root, "method");
        json_t *juser = json_object_get(root, "username");

        if (json_is_string(method) && strcmp(json_string_value(method), "login") == 0 &&
            json_is_string(juser)) {
            const char* incomingUsername = json_string_value(juser);
            if (is_username_unique(incomingUsername)){
                strncpy(username, json_string_value(juser), sizeof(username) - 1);
                printf("%s è connesso al server\n",username);
                json_decref(root);
                break;
            } else {
                const char *duplicate_msg = "{\"response\":\"error\",\"message\":\"Username già in uso, scegline un altro\"}\n";
                send(client_sock, duplicate_msg, strlen(duplicate_msg), 0);
            }
            
        } else {
            send(client_sock, login_request, strlen(login_request), 0);
        }

        json_decref(root);
    }

    // Aggiungi client
    pthread_mutex_lock(&clients_mutex);
    if (client_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clients_mutex);
        close(client_sock);
        return NULL;
    }
    clients[client_count].sock = client_sock;
    strncpy(clients[client_count].username, username, sizeof(clients[client_count].username) - 1);
    client_count++;
    pthread_mutex_unlock(&clients_mutex);

    // Notifica ingresso
    json_t *notif = json_object();
    json_object_set_new(notif, "event", json_string("user_joined"));
    json_object_set_new(notif, "username", json_string(username));
    broadcast_json(notif, client_sock);
    json_decref(notif);

    // Loop ricezione messaggi
    int bytes_received;
    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        route_request(client_sock, username, buffer);
    }

    remove_client(client_sock);
    remove_games(username);
    notify_disconnection(username);
    close(client_sock);
    return NULL;
}

//MAIN

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        int *client_sock = malloc(sizeof(int));
        if (!client_sock) continue;

        *client_sock = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (*client_sock < 0) {
            perror("accept failed");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, client_sock) != 0) {
            perror("pthread_create");
            close(*client_sock);
            free(client_sock);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
