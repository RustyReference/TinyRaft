#ifndef SERVER_H
#define SERVER_H

#include "../include.h"

// just a useful thing to have really
// set index to -1 unless in a linked list
struct servInfo {
	struct sockaddr_in addr;
	socklen_t addrlen;
	int sockfd;
};

// init a server
// @serverInfo : where you want server info stored
// @port : port to listen at
// #RETURN : socket and address of the leader-server
// 	0 at error. Can use perror after return.
// 	1 on success.
int startServer(struct servInfo* serverInfo, uint32_t port);

// process a leader server
// @serverInfo : uses the socket and address from this
// @clientPassword : password for clients. They can read and write to the db.
// @backupPassword : password for backups. They can read and write logs.
// #RETURN : returns once server is exited. Don't forget to close all sockets.
// 	0 on error.
// 	1 on success. 
// #GLOBALS:
// 	endLeaderServer : set 0 to end leader server process. 
int processLeader(struct servInfo serverInfo, char* clientPassword, char* backupPassword);

// safely get some data from a server. 
// @server : data is recieved FROM this server. 
// @buf : buffer to store the message at.
// @bufsiz : lenght of the buffer
// @timeout : wait maximum of timeout microseconds.
// #RETURN : lenght of message recieved
// 	-1 on timeout error
// 	-2 on select() error (probably wrong socket etc)
// 	-3 on recv() error (is buf NULL)
// 	otherwise lenght of the input (including 0)
int srecv(struct servInfo server, char* buf, size_t bufsiz, int timeout);

// When a client server first connects this is called.
// @server : info about client server. Socket, address, etc.
// @buf : first message client sent
// @bufsiz : max size of said message
// @pass : client password
// @servers : linked list of all servers for you to add to 
void handleClientInitial(struct servInfo server, char* buf, int bufsiz, char* pass, struct llist* servers);

// When a client server first connects this is called.
// @server : info about client server. Socket, address, etc.
// @buf : first message client sent
// @bufsiz : max size of said message
// @pass : client password
// @servers : linked list of all servers for you to add to 
void handleBackupInitial(struct servInfo server, char* buf, int bufsiz, char* pass, struct llist* servers);

// placeholder functions
void handleClients(struct llist* clientServers, struct llist* backupServers, char* pass);
void handleBackups(struct llist* servers, char* pass);

int getServer(struct servInfo* buf, int sockfd, int timeout);
#endif
