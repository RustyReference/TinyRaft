#ifndef NODE_H
#define NODE_H

/**
 * Contains static configuration about each node when the program begins
 * 
 * Postcondition
 */
typedef struct {
    int id;
    int port;
    char **peers;
    int num_peers;
} Config; 

#endif