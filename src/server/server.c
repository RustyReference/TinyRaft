#ifndef LEADER_H
#define LEADER_H

#include "server.h"
#include <sys/time.h>
#include <arpa/inet.h>
#ifndef MAXID 
#define MAXID 16
#endif

// A pool of unique ids.
struct {
	int ids[MAXID];
	pthread_mutex_t lock;
} idpool = { 0 };

// list of servers
struct ServListSafe clientList, backupList;
struct ServThread* leader = NULL;

// global index
struct Index indexer;

// init all startup stuff and global variables. 
void initServer(void) {
	indexer.ind = 0;
    	pthread_mutex_init(&indexer.lock, NULL);
	// server list
	SLIST_INIT(&clientList.servers);
	pthread_mutex_init(&clientList.lock, NULL);
	SLIST_INIT(&backupList.servers);
	pthread_mutex_init(&backupList.lock, NULL);

	// id pool
	memset(idpool.ids, 0, sizeof idpool.ids);
	pthread_mutex_init(&idpool.lock, NULL);
}

/**
 * Initializes the global command indexing variable
 *
void initIndex() {
	indexer.ind = 0;
    pthread_mutex_init(&indexer.lock, NULL);
}
*/

// terminate all startup stuff.
void termServer(void) {
	// clientlist
	pthread_mutex_lock(&clientList.lock);
	struct ServListEntry* np = SLIST_FIRST(&clientList.servers);
	while(np) {
		struct ServListEntry* temp = SLIST_NEXT(np, servers);
		ServThreadFree(&np->server);
		free(np);
		np = temp;
	}

	// backuplist
	pthread_mutex_lock(&backupList.lock);
	np = SLIST_FIRST(&backupList.servers);
	while(np) {
		struct ServListEntry* temp = SLIST_NEXT(np, servers);
		// free(np);
		ServThreadFree(&np->server);
		free(np);
		np = temp;
	}
	// indexer
	//free(indexer.ind)
	pthread_mutex_destroy(&indexer.lock);
	pthread_mutex_destroy(&backupList.lock);
	pthread_mutex_unlock(&backupList.lock);

	// idpool
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


// Init and start listening for a leader server.
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
	leader = server;
	struct ThreadMsg* coms = server->coms;

	// while getting messages
	char* buf = NULL;
	int len = 0;
	while((len = threadMsgRecv(coms, &buf)) >= 0) {
		if(!leaderCommandExec(buf, len)) {
			break;
		}
		free(buf);
		buf = NULL;
	}
	free(buf);
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

	leader = server; // global loopback
	return leader;
}

// free ServThread the pointer @server from memory
void ServThreadFree(struct ServThread** server) {
	struct ServThread* contents = *server;
	struct ThreadMsg* coms = contents->coms;

	int sockfd = contents->info.sockfd;

	// free stored pointers
	close(sockfd);
	clearId(contents->id);
	//pthread_mutex_lock(&coms->mlock);
	
	// cancel threads
	for(int i = 0; i < contents->tlen; i++) {
		pthread_cancel(contents->tid[i]);
		pthread_join(contents->tid[i], NULL);
	}

	// free mutex
//	pthread_mutex_unlock(&coms->mlock);
	pthread_mutex_destroy(&coms->mlock); // for the freebsd users, all 2 of them.
	ThreadMsgFree(&coms);

	// free structs 
	free(contents->tid);
	free(contents);
	*server = NULL;
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

	// free copy
	return i;
}

/** Parse and execute command from a sent message
 * @msg : The message sent from a server thread
 * @msglen : Length of the message
 * #RETURN : -1 on failure and 0 on success
 */

int parseAndExecute(char* msg, int msglen) {
	printf("Parsing command: %s\n", msg);

	char* buf[255];
	int numParts = strnsplit(msg, msglen, ' ', buf);
	
	if(numParts<=0) {
		printf("ERROR: Failed to parse command\n");
		return -1;
	}
	printf("Message has %d parts\n", numParts);
	// Get the cmd type from first word
	char* cmdtype = buf[0];
	if(strncmp(cmdtype, "write", 5)==0) {
		printf("WRITING to database\n");
		char* key = buf[1];
		char* val = buf[3];
		put(key, val);
	}
	else if(strncmp(cmdtype, "read", 5)==0) {
		printf("READING data from %d\n", cmdtype[1]);
	}

	for(int i = 0; i<numParts; i++) {
		printf("buf[%d] = %s\n", i, buf[i]);

	}

	free(*buf);
	return 0; // Successfully parsed
}



// execute a command for the leader
// @cmd : Command
// @cmdlen : maxsize of the command
// #RETURN : negative on error, otherwise ID of the command.
// 	-1 : invalid/error
// 	0 : exit -> stop leader server
// 	1 : backup-all -> say something to all backups at once
// 	2 : client-all -> say something to all clients at once
// 	3 : backup-address -> say something to a specific backup server ( work in progress )
// 	4 : client-address -> say something to a specific client sever ( work in progress )
int leaderCommandExec(char* cmd, int cmdlen) {
	// sanitize
	if(!cmd || cmdlen <= 0) {
		return -1;
	} 
	
	// pretty well optimized so might as well
	char* buf[255];
	int arglen = strnsplit(cmd, cmdlen, ' ', buf);
	if(arglen < 0) {
		free(*buf);
		return -1;
	}
	int firstlen = strnlen(buf[0], cmdlen)+1;

	// redirections
	cmd += firstlen;
	
	if(strncmp(buf[0], "write", 5)==0 ||
	   strncmp(buf[0], "read", 4)==0 ||	
	   strncmp(buf[0], "delete", 6)==0) {

		// directly parse and execute command
		cmd -= firstlen;
		int res = parseAndExecute(cmd, cmdlen);
		if(res<=0) {
			printf("ERROR Failed to parse command");
			return -1;
		}
		free(*buf);
		return 3;
	}
	// exit
	if(strncmp(buf[0], "exit", firstlen) == 0) {
		free(*buf);
		return 0;
	}

	// backup-all
	if(strncmp(buf[0], "backup-all", firstlen) == 0) {
		broadcastMsg(backupList, cmd, 0);
		leaderCommandExec(cmd, strnlen(cmd, cmdlen)+1); // recursion at its finest!
		free(*buf);
		return 1;
	}

	// client-all
	if(strncmp(buf[0], "client-all", firstlen) == 0) {
		broadcastMsg(clientList, cmd, 0);
		free(*buf);
		return 2;
	}

	// end of redirections.
	cmd -= firstlen;
	printf("%s\n", cmd);

	free(*buf);
	return -1;
}

// broadcast a message to a list of servers
// @server : list of servers
// @msg : what to say
// @maxlen : size of the message, set <= 0 for strnlen
void broadcastMsg(struct ServListSafe serverlist, char* msg, int maxlen) {
	// sanitize 
	if(!msg) {
		return;
	}
	if(maxlen < 0) {
		maxlen = 0;
	}

	// loop through while sending
	pthread_mutex_lock(&serverlist.lock);
	struct ServListEntry* current;
	// whoever made SLIST has my respect for this syntax
	SLIST_FOREACH(current, &serverlist.servers, servers) {
		threadMsgSend(current->server->coms, msg, 0);
	}
	// end
	pthread_mutex_unlock(&backupList.lock);
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
	socklen_t addrlen = 0;
	struct sockaddr_in6 addr = {0};
	int sockfd;
	
	// For each new connection.
	while((sockfd = accept(server->info.sockfd, (struct sockaddr*)&addr, &addrlen)) > 0) {
		// Gather the information
		struct ServInfo* info = malloc(sizeof(struct ServInfo));
		*info = (struct ServInfo){
			.sockfd = sockfd,
			.addr = addr,
			.addrlen = addrlen
		};

		// Start a new thread with it.
		pthread_t tid;
		pthread_create(&tid, NULL, leaderAddServer, info);
		pthread_detach(tid);
	}
	
	pthread_exit(NULL);
}

// add a server and start processing it based on backup or client
// @servThread : pointer to the ServThread struct with info and tid setup before
// #RETURN : NULL
void* leaderAddServer(void* servInfo) {
	int *oldtype = NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype); 

	// init
	struct ServThread* servThread = malloc(sizeof(struct ServThread));
	servThread->info = *(struct ServInfo*)servInfo;
	int sockfd = servThread->info.sockfd;
	free(servInfo);

	// timeout for recv
	const struct timeval tv = {
		.tv_sec = 1,
		.tv_usec = 0
	};
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	// get first message to see if valid
	char firstMsg[255];
	int status = recv(sockfd, firstMsg, 255, 0);
	if(status <= 0) {
		// timeout
		free(servThread);
		pthread_exit(NULL);
	}

	if(strncmp(firstMsg, "backup", 255) == 0) {
		// add to backupList and process it
		backupRecv(servThread);
	} 

	if(strncmp(firstMsg, "client", 255) == 0) {
		// add to backupList and process it 
		clientRecv(servThread);
	} 
	pthread_exit(NULL);
}

// Recieve communications from a command to process backupThread. 
// Will automatically free backupThread once backup disconnects, error or otherwise.
// @backupThread : ServThread with info ready. Will steal the pointer.
// #NOTES
// 	Steals the one and only pointer, do not free anything.
void backupRecv(struct ServThread* backupThread) {
	struct ServListEntry* np = addServListSafe(&backupList, backupThread);
	// set an id and start command thread
	backupThread->id = getId();
	backupThread->tid = calloc(2, sizeof(pthread_t));
	pthread_create(&backupThread->tid[0], NULL, backupCommandThread, backupThread);
  backupThread->tid[1] = pthread_self();
	backupThread->tlen = 2;

	// set coms
	backupThread->coms = ThreadMsgCreat();

	// no timeout for recv
	int sockfd = backupThread->info.sockfd;
	const struct timeval tv = {
		.tv_sec = 0,
		.tv_usec = 0
	};
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	// start recieving requests
	char buf[1024];
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	while(recv(sockfd, buf, 1024, 0) > 0) {
		*strchrnul(buf, '\n') = '\0';
		printf("BACKUP: Received command: %s\n", buf);

		// Parse and execute the command
		int result = parseAndExecute(buf, strlen(buf));
		if(result==-1) {
			printf("ERROR parsing and executing");
		}
		else{
			printf("COMMAND parsed and executed");
		}
	}

	// Remove from list and exit
	pthread_mutex_lock(&backupList.lock);
	SLIST_REMOVE(&backupList.servers, np, ServListEntry, servers);
	free(np);
	pthread_mutex_unlock(&backupList.lock);
	backupThread->tlen--;
	ServThreadFree(&backupThread);
  pthread_exit(NULL);
}

// Start processing commands for a backupThread in their own thread.
// @backupThread : ServThread fully setup and ready.
// #RETURN : NULL
// NOTES 
// 	Communicate while running using the coms inside backupThread.
void* backupCommandThread(void* backupThread) {
	int *oldtype = NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype); 

	// init
	struct ServThread* thread = backupThread;
	struct ThreadMsg* coms = thread->coms;
	char* buf = NULL;
	int len = 0;
	while( (len = threadMsgRecv(coms, &buf) >= 0) ) {
		// just necessary for backupCommandThread only for some reason.
    // SHIVESH : encrypt(buf, buflen);
		send(thread->info.sockfd, buf, strnlen(buf, 1024)+1, 0); // echo directly to the backup server
		free(buf);
	}
	thread->tlen--;
  thread->tid[0] = thread->tid[1]; // ?
	pthread_exit(NULL);
}

// Add servThread to servList.
// @servList : clientList or backupList.
// @servThread : the thread complete with tid and all.
// #RETURN : NULL on error and entry on success
// NOTES
// 	This function steals the servThread pointer. You may not free it anymore. 
// 	DO NOT FREE SERVTHREAD AFTER ITS BEEN ADDED WITHOUT REMOVING FIRST.
struct ServListEntry* addServListSafe(struct ServListSafe* servList, struct ServThread* servThread) {
	// sanitize
	if(!servList || !servThread) {
		return NULL;
	}

	// init 
	pthread_mutex_lock(&servList->lock);
	struct ServListEntry* entry = malloc(sizeof(struct ServListEntry));
	entry->server = servThread;
	SLIST_INSERT_HEAD(&servList->servers, entry, servers);
	pthread_mutex_unlock(&servList->lock);

	// exit
	return entry; 
}

// Recieve communications from a command to process clientThread. 
// Will automatically free clientThread once backup disconnects, error or otherwise.
// @clientThread : ServThread with info ready. Will steal the pointer.
// #NOTES
// 	Steals the one and only pointer, do not free anything.
void clientRecv(struct ServThread* clientThread) {
	struct ServListEntry* np = addServListSafe(&clientList, clientThread);
	// set an id and start command thread
	clientThread->id = getId();
	clientThread->tid = calloc(2, sizeof(pthread_t));
	pthread_create(&clientThread->tid[0], NULL, clientCommandThread, clientThread);
  clientThread->tid[1] = pthread_self();
	clientThread->tlen = 2;

	// set coms
	clientThread->coms = ThreadMsgCreat();

	// no timeout for recv
	int sockfd = clientThread->info.sockfd;
	const struct timeval tv = {
		.tv_sec = 0,
		.tv_usec = 0
	};
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	// start recieving requests
	char buf[1024];
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	while(recv(sockfd, buf, 1024, 0) > 0) {
		*strchrnul(buf, '\n') = '\0';
		// TODO: clinetCommandExec
		//printf("client at %s says %s\n", "address", buf);
		threadMsgSend(leader->coms, buf, 0);
	}

	// Remove from list and exit
	pthread_mutex_lock(&clientList.lock);
	SLIST_REMOVE(&clientList.servers, np, ServListEntry, servers);
	pthread_mutex_unlock(&clientList.lock);
	ServThreadFree(&clientThread);
  clientThread->tlen--;
	pthread_exit(NULL);
}

// Start processing commands for a clientThread in their own thread.
// @clientThread : ServThread fully setup and ready.
// #RETURN : NULL
// NOTES 
// 	Communicate while running using the coms inside clientThread.
void* clientCommandThread(void* clientThread) {
	int *oldtype = NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype); 

	// init
	struct ServThread* thread = clientThread;
	struct ThreadMsg* coms = thread->coms;
	char* buf = NULL;
	int len = 0;
	while( (len = threadMsgRecv(coms, &buf) >= 0) ) {
		// just necessary for clientCommandThread only for some reason.
    send(thread->info.sockfd, buf, strlen(buf)+1, 0);
		free(buf);
	}
	thread->tlen--;
  thread->tid[0] = thread->tid[1]; // ?
	pthread_exit(NULL);
}

#endif
