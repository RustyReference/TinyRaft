#ifndef CONFIG_H
#define CONFIG_H
#define ADDR_LEN 15

/**
 * Contains static configuration about each node when the program begins
 * 
 * Postcondition:
 * --This struct will contain pointers to dynamic memory
 */
typedef struct {
    int id;
    int port;
    char (*peers)[ADDR_LEN]; // Pointer to strings of length ADDR_LEN
    int num_peers;
} Config;

#endif