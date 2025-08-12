#ifndef PEERS_H
#define PEERS_H
#define IP_LEN 16
#include "config.h"

/**
 * Contains information about each peer the node has
 */
typedef struct {
    char ip[IP_LEN];
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
 * Parse string with IP Address and Port and 
 * read them into fields of a Peer, namely 'ip' and 'port'
 * @param ip_port the provided string in the form IP:PORT
 * @param ip the char pointer to the IP address 
 * @param port the pointer to the port
 * @return 0 on success, 1 otherwise
 */
int scan_peer_fields(char *ip_port, char *ip, int *port);

/**
 * Gets the peers stored in the Config object and 
 * @return a pointer to an array of them.
 */
Peer *get_peers(Config *config);

#endif