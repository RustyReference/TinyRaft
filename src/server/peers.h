#ifndef PEERS_H
#define PEERS_H
#define ADDR_LEN 15
#include <config.h>

/**
 * Contains information about each peer the node has
 */
typedef struct {
    char ip[ADDR_LEN];
    int port;
    int socket_fd;
} Peer;

/**
 * A dynamic peer array that contains length
 */
typedef struct {
    Peer *arr;
    int length;
} Peer_arr;

/**
 * @return pointer to a newly allocate Peer
 * 
 * Postcondition:
 *  --Returns dynamic memory that must be freed.
 */
Peer *create_peer();

/**
 * Gets the peers stored in the Config object and 
 * @return a pointer to an array of them.
 */
Peer *get_peers(Config *config);

#endif