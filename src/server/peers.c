#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "peers.h"
#include "config.h"

/**
 * @return pointer to a newly allocate Peer
 * 
 * Postcondition:
 *  --Returns dynamic memory that must be freed.
 */
Peer *create_peer() {
    errno = 0;
    Peer *peer = (Peer *) malloc(sizeof(Peer));
    if (peer == NULL) {
        fprintf("create_peer(): Failed to allocate memory: %s", 
            strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    return peer;
}

/**
 * Allocates memory for a NUMBER OF peers to assign to a dynamic array
 * @param length of the array
 * @return a pointer to the first element in the array
 */
Peer *alloc_peers(int length) {
    errno = 0;
    Peer *arr = (Peer *) calloc(length, sizeof(Peer));
    if (arr == NULL) {
        fprintf(stderr, "alloc_peers(): failed to allocate Peer array: %s", 
            strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    return arr;
}

/**
 * Parse string with IP Address and Port and 
 * read them into fields of a Peer, namely 'ip' and 'port'
 * @param ip_port the provided string in the form IP:PORT
 * @param ip the char pointer to the IP address 
 * @param port the pointer to the port
 */
void scan_peer_fields(char *ip_port, char *ip, int *port) {
    #define NUM_PARAMS 2
    errno = 0;
    int possible_error = sscanf(ip_port, "%[^:]:%d", ip, port);
    if (possible_error > 0 && possible_error < NUM_PARAMS) {
        fprintf(stderr, "\n%s%s", 
            "scan_peer_fields(): WARNING: not all fields", 
            " of a Peer have been successfully converted.\n");
        exit(EXIT_FAILURE);
    } else if (possible_error == 0) {
        fprintf(stderr, "\nscan_peer_fields(): Matching failure.\n");
        exit(EXIT_FAILURE);
    } else if (possible_error == EOF) {
        fprintf(stderr, "scan_peer_fields(): Read error: %s", 
            strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 * Gets the peers stored in the Config object and 
 * 
 * Postcondition:
 *      - RETURNS DYNAMIC MEMORY
 * 
 * @return a pointer to an array of them.
 */
Peer *get_peers(Config *config) {
    int num_peers = config->num_peers;
    Peer *peers = alloc_peers(num_peers);
    
    // Access the peers
    char **config_peers = config->peers;
    for (int i = 0; i < num_peers; i++) {
        // Get the ip and port from each peer string and store in fields
        scan_peer_fields(
            config_peers[i], 
            peers[i].ip, 
            peers[i].port);
    }

    return peers;
}