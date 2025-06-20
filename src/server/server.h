#ifndef SERVER_H

#define SERVER_H
#define _Nullable

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/types.h>

// init all startup stuff and global variables . 
void initServer(void);

// terminate all startup stuff.
void termServer(void);

// get first available id. -1 on error.
int getId();

// free @id back to the pool.
void clearId(int id);

// Pass message between threads with this.
struct QueueEntry {
	STAILQ_ENTRY(QueueEntry) messages;
	char* buf;
	int len;
};
STAILQ_HEAD(MsgQueue, QueueEntry);

// Hold information about a server.
struct ServInfo {
	int sockfd;
	struct sockaddr_in6 addr;
	socklen_t addrlen;
};

// To send messages accross threads.
struct ThreadMsg {
	struct MsgQueue* mqueue; 				// message buffers.
	pthread_cond_t ready; 					// new message ready.
	// action of locking itself counts as a write! Don't forget.
	pthread_mutex_t mlock; 	 				// Don't modify without a lock!
};
struct ThreadMsg* ThreadMsgCreat();

// Use to manage servers as threads.
struct ServThread {
	struct ServInfo info; 	// socket and address
	struct ThreadMsg* coms; // commands
	pthread_t* tid; 				// thread ids
	int tlen; 							// thread amount.
	int id; 								// unique id of the server. 
};
void ServThreadFree(struct ServThread** server); // free a ServThread* server. 

// Get a QueueEntry. NULL on error.
// @msg : The initial string buffer.
// @len : max lenght of the buffer.
// 	Set len <= 0 for default 1024 maxlen 
// 	Set msg NULL for empty string.
struct QueueEntry* QueueEntryCreat(char* _Nullable msg, int len);

// Free the QueueEntry msg. Won't crash on NULL.
void QueueEntryFree(struct QueueEntry** _Nullable msg);

// Get an MsgQueue. NULL on malloc error.
// 	Don't forget to MsgQueueFree later.
struct MsgQueue* MsgQueueCreat(void);

// Free an MsgQueue
// mqueue : Pointer to destroy and free.
void MsgQueueFree(struct MsgQueue** _Nullable mqueue);

// Add a new message to the MsgQueue mqueue.
// @msg : Buffer to copy. Set NULL for empty.
// @len : maxlen. Set 0 or less for default 1024.
// #RETURN : 0 on malloc error. 1 on success.
int MsgQueuePush(struct MsgQueue* mqueue, char* msg, int len);

// Send message to a server thread.
// @coms : Appends message to commands in @coms.
// @msg : What to append.
// @len : Lenght of the command. <= 0 means @len = strnlen(msg, 1024);
// 	Will lock the thread and send notification. Return 0 on error.
int threadMsgSend(struct ThreadMsg* coms, char* msg, int len);

// Safely get message from a ThreadMsg
// @coms : Will use messagequeue from here.
// @buf : Will store message here. Does NOT copy. Free later.
// #RETURN 
// 	How many bytes buf is. -1 on critical error.
int threadMsgRecv(struct ThreadMsg* coms, char** buf);

// Free a ThreadMsg. No threads must be reading it.
void ThreadMsgFree(struct ThreadMsg** threadMsg);


#undef _Nullable
#endif
