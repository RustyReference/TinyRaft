#ifndef SERVER_H
#define LEADER_H

#include <unistd.h>
#include <sys/queue.h>
#include "thread/thread.h"

// Hold information about a server.
struct ServInfo {
	int sockfd;
	struct sockaddr_in6 addr;
	socklen_t addrlen;
};

// Use to manage servers as threads.
struct ServThread {
	struct ServInfo info; 	// socket and address. Don't change it.
	struct ThreadMsg* coms; // commands
	pthread_t* tid; 				// thread ids
	int tlen; 							// thread amount.
	int id; 								// unique id of the server. 
};
void ServThreadFree(struct ServThread** server); // free a ServThread* server. 

// list of servers with mutex
struct ServListEntry {
	struct ServThread* server;
	SLIST_ENTRY(ServListEntry) servers;
};
SLIST_HEAD(ServList, ServListEntry);

struct ServListSafe {
	struct ServList servers;
	pthread_mutex_t lock;
};

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

// Accept a new server and start its own thread
// @leaderServer : ServThread of the leaderServer.
// #RETURN : NULL on exit.
void* leaderAcceptThread(void*);

// add a server and start processing it based on backup or client
// @servThread : pointer to the ServThread struct with info and tid setup before
void* leaderAddServer(void* servThread);

// split a string into multiple strings
// @str : string you wish to split.
// @len : max lenght of the str
// @delim : delimiter to use when splitting.
// @buf : Will store the strings in this array.
// #RETURN : lenght of array
// 	-1 on error
// #NOTES :
// 	free *buf once without looping over once done, ex.
// 	char* buf[255];
// 	int len = strnsplit("hello there how is it going", 1024, ' ', buf);
// 	if(len > 0) {
// 		...
// 		free(*buf);
// 	}
int strnsplit(char* str, int len, char delim, char* buf[]);

int backupCommandExec(struct ServThread* server, char* cmd, int cmdlen);

// broadcast a message to a list of servers
// @server : list of servers
// @msg : what to say
// @maxlen : size of the message, set <= 0 for strnlen
void broadcastMsg(struct ServListSafe serverlist, char* msg, int maxlen);

// execute a command for the leader
// @cmd : Command
// @cmdlen : maxsize of the command
// #RETURN : negative on error, otherwise ID of the command.
// 	-1 : invalid/error
// 	0 : exit command
// 	ID of the command just executed otherwise.
int leaderCommandExec(char* cmd, int cmdlen);

// Start recieving on a ServThread's socket and execute incoming requests
// @backupThread : pointer to a servThread with info initialized beforehand.
// #RETURN : Nothing
// NOTES : Will start a backupCommandThread and free everything once socket closes.
void backupRecv(struct ServThread* backupThread);

// Execute backup commands for a thread
// @backupThread : Thread to recieve commands for.
// #RETURN : NULL on exit
// NOTES
// 	backupRecv has an example usage. In fact use backupRecv what are you looking at this function for?
void* backupCommandThread(void* backupThread);

// Add servThread to servList.
// @servList : clientList or backupList.
// @servThread : the thread complete with tid and all.
// #RETURN : 0 on error and 1 on success
// NOTES
// 	This function steals the servThread pointer. You may not free it anymore. DO NOT FREE SERVTHREAD AFTER ITS BEEN ADDED.
//
int addServListSafe(struct ServListSafe* servList, struct ServThread* servThread);

#endif
