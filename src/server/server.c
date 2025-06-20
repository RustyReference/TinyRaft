#include "server.h"

#ifndef MAXID 
#define MAXID 16
#endif

// A pool of unique ids.
struct {
	int ids[MAXID];
	pthread_mutex_t lock;
} idpool;

// Get an MsgQueue. NULL on malloc error.
// 	Don't forget to MsgQueueFree later.
struct MsgQueue* MsgQueueCreat(void) {
	// mallocate the MsgQueue mqueue
	errno = 0;
	struct MsgQueue* mqueue = malloc(sizeof(struct MsgQueue));
	if(!mqueue) {
		errno = ENOMEM;
		return NULL;
	}
	// init mqueue and return.
	STAILQ_INIT(mqueue);
	return mqueue;
}

// Free an MsgQueue
// mqueue : Pointer to destroy and free.
void MsgQueueFree(struct MsgQueue** mqueue) {
	errno = 0;
	// skip to end if invalid
	if(!mqueue) {
		errno = EDESTADDRREQ;
		return;
	}
	// for each element temp in mqueue and set NULL.
	struct QueueEntry* temp; 
	struct QueueEntry* msg = STAILQ_FIRST(*mqueue);
	while(msg) {
		temp = STAILQ_NEXT(msg, messages);
		free(msg->buf);
		free(msg);
		msg = temp;
	}
	free(*mqueue);
}

// Get a QueueEntry.
// @msg : The initial string buffer.
// @len : max lenght of the buffer.
// 	Set len <= 0 for default 1024 maxlen and msg NULL for empty string.
// #RETURN : new QueueEntry on success. NULL if malloc fails.
// 	Don't forget to call QueueEntryFree
struct QueueEntry* QueueEntryCreat(char* msg, int len) {
	errno = 0;
	// init
	struct QueueEntry* entry = malloc(sizeof(struct QueueEntry));
	if(!entry) {
		errno = ENOMEM;
		return NULL;
	}
	// set lenght
	if(!len) { 
		len = 1024;
	}
	entry->len = len;
	// set message
	entry->buf = calloc(len, sizeof(char));	
	if(!msg) {
		entry->buf[0] = '\0';
	}
	strncpy(entry->buf, msg, len);
	// return
	return entry;
}

// Free the QueueEntry msg. Won't crash on NULL.
void QueueEntryFree(struct QueueEntry** msg) {
	errno = 0;
	// free and set to NULL
	if(!*msg) {
		errno = EDESTADDRREQ;
		return;
	}
	free((*msg)->buf);
	free(*msg);
}

// Add a new message to the MsgQueue mqueue.
// @msg : Buffer to copy. Set NULL for empty.
// @len : maxlen. Set 0 or less for default.
// #RETURN : 0 on malloc error. 1 on success.
int MsgQueuePush(struct MsgQueue* mqueue, char* msg, int len) {
	// sanitize
	if(!mqueue) {
		return 0;
	}
	// get new commands
	struct QueueEntry* entry = QueueEntryCreat(msg, len);
	if(!entry) {
		return 0;
	}
	// add it to mqueue and return
	STAILQ_INSERT_TAIL(mqueue, entry, messages);
	return 1;
}

// get first available id.
int getId() {

	int res = -1;
	pthread_mutex_lock(&idpool.lock);
	// get first index with zero-value
	for(int i = 0; i < MAXID; i++) {
		if(!idpool.ids[i]) {
			idpool.ids[i] = 1;
			res = i;
			break;
		}
	}
	pthread_mutex_unlock(&idpool.lock);
	// i on success and -1 on error.
	return res;
}

// free @id back to the pool.
void clearId(int id) {
	pthread_mutex_lock(&idpool.lock);
	idpool.ids[id] = 0;
	pthread_mutex_unlock(&idpool.lock);
}

// init all startup stuff and global variables. 
void initServer(void) {
	memset(idpool.ids, 0, sizeof idpool.ids);
	pthread_mutex_init(&idpool.lock, NULL);
}

// terminate all startup stuff.
void termServer(void) {
	pthread_mutex_destroy(&idpool.lock);
}

// free ServThread the pointer @server from memory
void ServThreadFree(struct ServThread** server) {
	struct ServThread* contents = *server;
	struct ThreadMsg* coms = contents->coms;
	int sockfd = contents->info.sockfd;
	// free stored pointers
	close(sockfd);
	clearId(contents->id);
	MsgQueueFree(&coms->mqueue);
	free(coms);
	// free struct
	for(int i = 0; i < contents->tlen; i++) {
		pthread_cancel(contents->tid[i]);
		pthread_join(contents->tid[i], NULL);
	}
	free(contents->tid);
	free(contents);
	*server = NULL;
}

// Send message to a server thread.
// @coms : Appends message to commands in @coms.
// @msg : What to append.
// @len : Lenght of the command. <= 0 means @len = strnlen(msg, 1024);
// 	Will lock the thread and send notification. Return 0 on error.
int threadMsgSend(struct ThreadMsg* coms, char* msg, int len) {
	errno = 0;

	// sanitize
	if(!coms || !msg) {
		errno = EINVAL;
		return 0;
	}
	if(len <= 0) {
		len = strnlen(msg, 1024)+1;
	}
	
	// lock
	pthread_mutex_lock(&coms->mlock);

	// modify
	if(!MsgQueuePush(coms->mqueue, msg, len)) {
		pthread_mutex_unlock(&coms->mlock);
		return 0;
	}
	pthread_cond_broadcast(&coms->ready); // I am in doubt.

	// unlock
	pthread_mutex_unlock(&coms->mlock);
	return 1;
}

const struct timespec ts = {
	.tv_nsec = 0,
	.tv_sec = 1
};
// Safely get message from a ThreadMsg
// @coms : Will use messagequeue from here.
// @buf : Will store message here. Does NOT copy.
// #RETURN 
// 	How many bytes buf is.
int threadMsgRecv(struct ThreadMsg* coms, char** buf) {
	// sanitize
	if(!coms || !buf) {
		return -1;
	}
	
	// lock
	pthread_mutex_lock(&coms->mlock);

	// wait for notification
	struct QueueEntry* entry = NULL;
	while(!entry) {
		pthread_cond_timedwait(&coms->ready, &coms->mlock, &ts);
		entry = STAILQ_FIRST(coms->mqueue);
	}

	// operate
	if(!(*buf = entry->buf)) {
		pthread_mutex_unlock(&coms->mlock);
		return -1;
	}
	int res = entry->len;

	// free rest
	STAILQ_REMOVE_HEAD(coms->mqueue, messages);
	free(entry);

	// unlock
	pthread_mutex_unlock(&coms->mlock);
	return res;
}

// get a new ThreadMsg. Call ThreadMsgFree later.
struct ThreadMsg* ThreadMsgCreat() {
	struct ThreadMsg* coms = (struct ThreadMsg*)malloc(sizeof(struct ThreadMsg));
	if(!coms) {
		return NULL;
	}

	// setup the message queue
	coms->mqueue = MsgQueueCreat();
	if(!coms->mqueue) {
		// abort
		free(coms);
		return NULL;
	}

	// continue setting up communications
	pthread_cond_init(&coms->ready, NULL);
	pthread_mutex_init(&coms->mlock, NULL);
	return coms;
}

// Free a ThreadMsg. No threads must be reading it.
void ThreadMsgFree(struct ThreadMsg** threadMsg) {
	struct ThreadMsg* coms = *threadMsg;
	// sanitize
	if(!coms) {
		return;
	}

	// remove mutexes.
	pthread_mutex_unlock(&coms->mlock);
	
	// For all the bsd users we have out there. All 2 of them.
	pthread_mutex_destroy(&coms->mlock); 	
	pthread_cond_destroy(&coms->ready);

	// free rest
	MsgQueueFree(&coms->mqueue);
	free(coms);
	*threadMsg = NULL;
}

// Start some threads for a server. 
// @server : Threads will be put on this ServThread
// @threadFunc : This function will be called with server as the only arg.
// #RETURN : 0 on error and 1 on success.
int servThreadAdd(struct ServThread* server, void* (threadFunctions)(void*), ...) {
	va_list ap;
	va_start(ap, threadFunctions);
	int i, err = -1;
	// make each function
	for(i = 0; *threadFunctions; i++) {
		void* nextFunction = va_arg(ap, void*);
		if(pthread_create(&server->tid[i], NULL, nextFunction, NULL)) {
			// abort
			err = i; // This is a headache...
			break;
		}
	}
	// don't really have to read this
	if(err >= 0) {
		// undo everything and exit
		for(int i = 0; i <= err; i--) {	
			pthread_cancel(server->tid[i]);
			pthread_join(server->tid[i], NULL);
		}
		return 0;
	}
	// success
	va_end(ap);
	return 1;
}
