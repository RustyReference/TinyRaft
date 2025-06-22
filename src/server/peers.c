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
 * Gets the peers stored in the Config object and 
 * @return a pointer to an array of them.
 */
Peer *get_peers(Config *config) {
    Peer_arr peers;
    
    // Access the peers
    char (*config_peers)[ADDR_LEN] = config->peers;
    for (int i = 0; i < config->num_peers; i++) {
        // Get the ip and port from each peer string
        (peers.arr)[i].ip = config_peers[i];
    }
}