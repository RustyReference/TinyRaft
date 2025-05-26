#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/* global variables (yippeee) */
int isLeader = 0;


/* For backup servers */

// Connect to a leader.
int connectLeader(char* addr, int port);


/* For leader servers */

// send changes to backup servers
int giveChanges(char* key, char* value);

// send logs to backup servers
int giveLogs(char* filepath);

// send database to backup servers
int givedb(char* filepath);


/* For both */ 

// init and start the server.
int initServer(int port, int leader, char pathLogs[255], char pathdb[255], char passwd[255]);

// write to db
int writedb(char* key, char* value);

#endif
