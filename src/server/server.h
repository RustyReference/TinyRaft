#ifndef SERVER_H
#define LEADER_H

#include "thread/thread.h"

// Hold information about a server.
struct ServInfo {
	int sockfd;
	struct sockaddr_in6 addr;
	socklen_t addrlen;
};

// Use to manage servers as threads.
struct ServThread {
	struct ServInfo info; 	// socket and address
	struct ThreadMsg* coms; // commands
	pthread_t* tid; 				// thread ids
	int tlen; 							// thread amount.
	int id; 								// unique id of the server. 
};
void ServThreadFree(struct ServThread** server); // free a ServThread* server. 

// init all startup stuff and global variables . 
void initServer(void);

// terminate all startup stuff.
void termServer(void);

// Init a leader server.
// @dst : Store the socket and address in @dst
// @port : Bind to port @port
// #RETURN : 0 on error. 1 on success.
int getLeader(struct ServInfo* dst, int port);

// process commands in a thread for some leader server
// @server : ServThread to use. Look at procLeader for example process.
// #RETURN : NULL on exit.
void* leaderCommandThread(void* leaderServer);

// start processing a leader server
// @info : get socket from @info.
// @RETURN : ThreadMsg. NULL on error. 
// 	tid[0] is used to process commands.
// 	tid[1] is used to accept new connections.
struct ServThread* procLeader(struct ServInfo info);

void* leaderAcceptThread(void*);

#endif
