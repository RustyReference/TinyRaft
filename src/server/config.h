#ifndef CONFIG_H
#define CONFIG_H
#define ADDR_LEN 21
#include "peers.h"

/**
 * Contains static configuration about each node when the program begins
 * 
 * Postcondition:
 * --This struct will contain pointers to dynamic memory
 */
typedef struct {
    int id;
    int port;
    int num_peers;
    char **peers;   // All peers should be at most ADDR_LEN long
} Config;

#endif