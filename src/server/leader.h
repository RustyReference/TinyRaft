#ifndef LEADER_H
#define LEADER_H

#include "server.h"

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

void* leaderAcceptThread(void*) {
	int *oldtype = NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);
	while(1);
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
	if(pthread_create(&server->tid[1], NULL, leaderAcceptThread, NULL)) {
		// abort
		clearId(id);
		ThreadMsgFree(&coms);
		free(server->tid);
		free(server);
		return NULL;
	}

	return server;
}

#endif
