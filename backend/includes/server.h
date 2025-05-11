#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

#define MAX_CLIENTS 20
#define MAX_GAMES 10
#define DEFAULT_PORT 8080
#define DISCONNECT_MESSAGE "!DISCONNECT"

typedef struct {
    ssize_t socket_fd;
    bool running;
    struct sockaddr_in address;
    pthread_mutex_t clients_mutex;
    pthread_mutex_t games_mutex;
} server_t;

/**
 * Inizializza le variabili utili a gestire le informazioni del server
 */
void server_init(server_t *server, int port);

/**
 * Crea una socket di tipo STREAM per il dominio TCP/IP, la imposta in modalità
 * SO_REUSEADDR in modo da consentire il riutilizzo degli indirizzi.
 */
bool server_start(server_t *server);

/**
 * Libera la memoria e imposta i campi della struttura ai valori di default prima di chiudere il server
 */
void server_close(server_t *server);

#endif