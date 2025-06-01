#include "server.h"
#include <signal.h>

int sockLeaderServer = -1;

void endServer(int sig) {
	if(sockLeaderServer >= 0) {
		close(sockLeaderServer);
	}
	exit(EXIT_SUCCESS);
}

// init a server
// @serverInfo : where you want server info stored
// @port : port to listen at
// #RETURN : socket and address of the leader-server
// 	0 at error. Can use perror after return.
// 	1 on success.
int startServer(struct servInfo* serverInfo, uint32_t port) {
	// init
	struct sockaddr_in addr;
	int sockfd, status, addrlen = sizeof(addr);

	// get a socket sockfd
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) { return 0; }
	sockLeaderServer = sockfd;
	signal(SIGINT, endServer);

	// get an address addr
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	// bind socket sockfd to address addr
	status = bind(sockfd, (struct sockaddr*)&addr, addrlen);
	if(status < 0) { return 0; }

	if(listen(sockfd, 5) < 0) { return 0; }

	// save to serverInfo and return 1
	serverInfo->sockfd = sockfd;
	serverInfo->addr = addr;
	serverInfo->addrlen = addrlen;
	return 1;
}

// process a leader server
// @serverInfo : uses the socket and address from this
// @clientPassword : password for clients. They can read and write to the db.
// @backupPassword : password for backups. They can read and write logs.
// #RETURN : returns once server is exited. Don't forget to close all sockets.
// 	0 on error.
// 	1 on success. 
// #GLOBALS:
// 	endLeaderServer : set 0 to end leader server process. 
int endLeaderServer = 0; // set to 0 to end leader server process
int processLeader(struct servInfo serverInfo, char* clientPassword, char* backupPassword) {
	// Master means "currently connected." Like "master key" but "master socket."
	char bufMaster[1024] = "";
	struct llist clientServers = llistCreat();
	struct llist backupServers = llistCreat();
	struct servInfo currentServer = {0};

	// main event loop for leaderServer
	char temp[1024];
	while(!endLeaderServer) {
		// get a server connection
		if(getServer(&currentServer, serverInfo.sockfd, 50000)) {
			// get some data and process if possible
			if(recv(currentServer.sockfd, bufMaster, 1024, 0) < 0) {
				perror("");
				close(currentServer.sockfd);
			} else if(strncmp(bufMaster, "client", 6) == 0) {
				printf("client\n");
				handleClientInitial(serverInfo, bufMaster, 1024, clientPassword, &clientServers);
			} else if(strncmp(bufMaster, "backup", 6) == 0) {
				printf("backup\n");
				handleBackupInitial(serverInfo, bufMaster, 1024, backupPassword, &backupServers);
			}
		}
		// process clientServers
		handleClients(&clientServers, &backupServers, clientPassword);
		handleBackups(&clientServers, backupPassword);
	}

	// success
	return 1;
}

// safely get some data from a server. 
// @server : data is recieved FROM this server. 
// @buf : buffer to store the message at.
// @bufsiz : lenght of the buffer
// @timeout : wait maximum of timeout microseconds.
// #RETURN : <0 for error status >= 0 for lenght of message recieved.
// 	-1 on timeout error
// 	-2 on select() error (probably wrong socket etc)
// 	-3 on recv() error (is buf NULL)
// 	otherwise lenght of the input (including 0)
int srecv(struct servInfo server, char* buf, size_t bufsiz, int timeout) {
	// init
	if(!buf) { return -3; }
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(server.sockfd, &rfds);
	struct timeval tv = {
		.tv_sec = timeout/1000, 
		.tv_usec = timeout%1000
	};

	// wait for input if not ready already
	int retval = select(server.sockfd+1, &rfds, NULL, NULL, &tv);
	
	// retval is 0 on timeout and -1 on select() error so...
	/*
	if(retval < 0) { 
		FD_CLR(server.sockfd, &rfds);
		return retval - 1;
	} 
*/
	// recieve data and regturn lenght
	int buflen = recv(server.sockfd, buf, bufsiz, 0);
	if(buflen < 0) { return -3; }
	return buflen;
}

// When a server wants to become a client this is what they have to go through.
void handleClientInitial(struct servInfo server, char* buf, int bufsiz, char* pass, struct llist* servers) {
	llistAdd(servers, &server);
}

// Once someone wants to become a backup this is what they have to go through.
void handleBackupInitial(struct servInfo server, char* buf, int bufsiz, char* pass, struct llist* servers) {
	llistAdd(servers, &server);
}

int getServer(struct servInfo* buf, int sockfd, int timeout) {
	// check the socket
	/*
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);
	struct timeval tv = {
		.tv_sec = timeout/1000, 
		.tv_usec = timeout%1000
	};
	int retval = select(sockfd+1, &rfds, NULL, NULL, &tv);

	// no incoming communications for timeout miliseconds...
	if(retval < 0) { 
		FD_CLR(sockfd, &rfds);
		return 0;
	} 
	*/

	// init
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int sockMaster = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
	if(sockMaster < 0) { return 0; }

	// store the info for later use
	*buf = (struct servInfo) {
		.sockfd = sockMaster, 
		.addr = addr, 
		.addrlen = addrlen,
	};

	// success
	return 1;
}

// placeholder function
void handleClients(struct llist* clientServers, struct llist* backupServers, char* pass) {
	struct node* head = clientServers->first;
	char buf[1024];
	while(head) {
		// print address and what they have to say
		struct servInfo* server = (struct servInfo*)head->data;
		if(srecv(*server, buf, 1024, 500) > 0) {
			printf("Client %s : %s\n", inet_ntoa(server->addr.sin_addr), buf);
			struct node* backupServer = backupServers->first;
			// share with all backup servers as well
			while(backupServer) {
				struct servInfo* backup = (struct servInfo*)head->data;
				send(backup->sockfd, buf, 1024, 0);
				backupServer = backupServer->next;
			}
		}
		head = head->next;
	}
}

// placeholder function
void handleBackups(struct llist* servers, char* pass) {
	struct node* head = servers->first;
	char buf[1024];
	while(head) {
		// print address and what they have to say
		struct servInfo* server = (struct servInfo*)head->data;
		if(srecv(*server, buf, 1024, 500) > 0) {
			printf("Backup %s : %s\n", inet_ntoa(server->addr.sin_addr), buf);
		}
		head = head->next;
	}
}
