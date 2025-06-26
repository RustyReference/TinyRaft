#ifndef LEADER_H
#define LEADER_H

#include "server.h"
#include <sys/time.h>
#include <arpa/inet.h>

#ifndef MAXID 
#define MAXID 16
#endif

struct ServList clientList;
struct ServList backupList;

// A pool of unique ids.
struct {
	int ids[MAXID];
	pthread_mutex_t lock;
} idpool;

// init all startup stuff and global variables. 
void initServer(void) {
	SLIST_INIT(&clientList);
	SLIST_INIT(&backupList);
	memset(idpool.ids, 0, sizeof idpool.ids);
	pthread_mutex_init(&idpool.lock, NULL);
}

// terminate all startup stuff.
void termServer(void) {
	pthread_mutex_destroy(&idpool.lock);
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


// Init a leader server.
// @dst : Store the socket and address in @dst
// @port : Bind to port @port
// #RETURN : 0 on error. 1 on success.
int getLeader(struct ServInfo* dst, int port) {
	errno = 0;

	// get a socket sockfd
	int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if(sockfd < 0) {
		dst = NULL;
		return 0;
	}

	// get rid of the pesky "bind address already in use" message.
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	// init info
	if(!dst) {
		errno = EDESTADDRREQ;
		dst = NULL;
		return 0;
	}

	// set address
	dst->addr = (struct sockaddr_in6){
		.sin6_family = AF_INET6,
		.sin6_port = htons(port),
		.sin6_flowinfo = 0,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_scope_id = 0
	};
	dst->addrlen = sizeof(dst->addr);

	// bind address to socket sockfd
	if(bind(sockfd, (struct sockaddr*)&dst->addr, dst->addrlen) < 0) {
		dst = NULL;
		return 0;
	}

	// start listening on address
	if(listen(sockfd, 5) < 0) {
		dst = NULL;
		return 0;
	}
	dst->sockfd = sockfd;
	return 1;
}

// process commands in a thread for some leader server
// @server : ServThread to use. Look at procLeader for example process.
// #RETURN : NULL on exit.
void* leaderCommandThread(void* leaderServer) {
	int *oldtype = NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype); 
	// sanitize
	if(!leaderServer) {
		pthread_exit(NULL);
	}
	// init
	struct ServThread* server = (struct ServThread*)leaderServer;
	struct ThreadMsg* coms = server->coms;

	// while getting messages
	char* buf = NULL;
	int len = 0;
	while((len = threadMsgRecv(coms, &buf)) >= 0) {
		if(strncmp("exit", buf, len) == 0) {
			// exit
			free(buf);
			buf = NULL;
			break;
		} else if(strncmp("greet", buf, len) == 0) {
			// greet : say "hello, world""
			printf("hello, world\n");
		} else if(strncmp("say", buf, len) == 0) {

		}
		free(buf);
		buf = NULL;
	}
	pthread_exit(NULL);
}

// start processing a leader server
// @info : get socket from @info.
// @RETURN : ThreadMsg. NULL on error. 
// 	tid[0] is used to process commands.
// 	tid[1] is used to accept new connections.
struct ServThread* procLeader(struct ServInfo info) {
	// get an id for the leader.
	int id = getId();
	if(id < 0) {
		// ...HOW??
		return NULL;
	}

	// setup commands 
	struct ThreadMsg* coms = ThreadMsgCreat();
	if(!coms) {
		// abort
		clearId(id);
		return NULL;
	}

	// get result
	struct ServThread* server = (struct ServThread*)malloc(sizeof(struct ServThread));
	if(!server) {
		// abort
		clearId(id);
		ThreadMsgFree(&coms);
		return NULL;
	}

	// set result
	*server = (struct ServThread){
		.info = info,
		.coms = coms,
		.tid = (pthread_t*)calloc(2, sizeof(pthread_t)),
		.tlen = 2, // 1 for commands and 1 more for good measure
		.id = id
	};

	// start the threads
	if(!server->tid) {
		// abort
		clearId(id);
		ThreadMsgFree(&coms);
		free(server);
		return NULL;
	}
	
	// start the threads
	if(pthread_create(&server->tid[0], NULL, leaderCommandThread, server)) {
		// abort
		clearId(id);
		ThreadMsgFree(&coms);
		free(server->tid);
		free(server);
		return NULL;
	}

	// DRY stands for "do repeat yourself"
	if(pthread_create(&server->tid[1], NULL, leaderAcceptThread, server)) {
		// abort
		clearId(id);
		ThreadMsgFree(&coms);
		free(server->tid);
		free(server);
		return NULL;
	}

	return server;
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

// Accept a new server and start its own thread
// @leaderServer : ServThread of the leaderServer.
// #RETURN : NULL on exit.
void* leaderAcceptThread(void* leaderServer) {
	// init
	int *oldtype = NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);
	struct ServThread* server = (struct ServThread*)leaderServer;
	
	// init bufs
	socklen_t addrlen;
	struct sockaddr_in6 addr;
	int sockfd;
	
	// For each new connection.
	while((sockfd = accept(server->info.sockfd, (struct sockaddr*)&addr, &addrlen))) {
		// Gather the information
		struct ServInfo info = {
			.sockfd = sockfd,
			.addr = addr,
			.addrlen = addrlen
		};
		// Start a new thread with it.
		struct ServThread* servThread = malloc(sizeof(struct ServThread));
		servThread->info = info;
		servThread->tid = calloc(2, sizeof(pthread_t));
		servThread->tlen = 2;
		pthread_create(&servThread->tid[0], NULL, leaderAddServer, servThread);
	}
	
	pthread_exit(NULL);
}

const struct timeval newServTimeout = {
	.tv_sec = 5,
	.tv_usec = 0
};

// Add a server to backupList or clientList and compute it.
// @servInfo : struct ServThread that had info, tid, and tlen initialized beforehand.
// #RETURN : NULL on exit.
void* leaderAddServer(void* servThread) {
	int *oldtype = NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);
	
	// set a heartbeat.
	struct ServThread* thread = (struct ServThread*)servThread;
	int sockfd = thread->info.sockfd;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &newServTimeout, sizeof(newServTimeout));

	// first connection must be made within 5 seconds.
	char buf[1024];
	if(recv(sockfd, buf, 1024, 0) < 0) {
		pthread_exit(NULL);
	}

	// init
	thread->coms = ThreadMsgCreat();
	thread->id = getId();
	if(strncmp(buf, "client", 1024) == 0) {
		// start processing as a client server
		pthread_create(&thread->tid[1], NULL, clientRecvThread, thread);
		clientCommandProc(thread);
		pthread_exit(NULL);
	} else if(strncmp(buf, "backup", 1024) == 0) {
		// start processing as a backup server
		pthread_create(&thread->tid[1], NULL, clientRecvThread, thread);
		backupCommandProc(thread);
	} else {
		// invalid
		thread->tlen--;
	}

	// free and exit
	ServThreadFree(&thread);
	pthread_exit(NULL);
}

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
int strnsplit(char* str, int len, char delim, char* buf[]) {
	if(!str) {
		return -1;
	}

	// skip start
	char* cpy = strndup(str, len);
	while(cpy[0] == delim) {
		cpy++;
	}
	buf[0] = cpy;

	// go around seperating stuff.
	int i;
	for(i = 1; *(cpy = strchrnul(cpy, delim)); i++ ) {
		// skip through delims
		while(*cpy == delim) {
			*cpy = '\0';
			cpy++;
		}
		// set element
		buf[i] = cpy;
	}
	buf[i+1] = NULL;
	
	return i;
}

void* clientRecvThread([[maybe_unused]]void* clientServer) {
	pthread_exit(NULL);
}
void clientCommandProc([[maybe_unused]]struct ServThread* clientServer) {
	return;
}
void* backupRecvThread([[maybe_unused]]void* backupServer) {
	pthread_exit(NULL);
}
void backupCommandProc([[maybe_unused]]struct ServThread* backupServer) {
	errno = 0;
	// sanitize
	if(!backupServer) {
		return;
	}

	// add to the list
	struct ServListEntry* entry = malloc(sizeof(struct ServListEntry));
	if(!entry) {
		errno = ENOMEM;
		free(backupServer);
		return;
	}
	entry->server = backupServer;
	SLIST_INSERT_HEAD(&backupList, entry, servers);

	// init
	struct ServInfo info = backupServer->info;
	const char* straddr = inet_ntop(AF_INET6, &info.addr.sin6_addr, NULL, sizeof(info.addr));
	struct ThreadMsg* coms = backupServer->coms;
	char* command[255] = { 0 };
	[[maybe_unused]]int commandLen = 0;

	// while getting messages
	char* buf = NULL;
	int len = 0;
	while((len = threadMsgRecv(coms, &buf)) >= 0) {
		// convert it to a command
		commandLen = strnsplit(buf, len, ' ', command);
		if(commandLen < 0) {
			// error
			continue;
		}
		if(strncmp("exit", buf, len) == 0) {
			// exit
			free(buf);
			buf = NULL;
			free(*command);
			*command = NULL;
			break;
		} else if(strncmp("greet", buf, len) == 0) {
			// greet : say "hello, world"
			printf("%s from %s\n", "hello, world", straddr);
		} else if(strncmp("say", buf, len) == 0) {
			// say : print something specific
			printf("%s from %s\n", buf+4, straddr);
		} else if(strncmp("send", buf, len) == 0) {
			// send : send this to the connected server
			send(info.sockfd, buf+5, len-5, 0);
		} 
		free(buf);
		buf = NULL;
		free(*command);
		*command = NULL;
	}
	pthread_exit(NULL);
	return;
}

#endif
