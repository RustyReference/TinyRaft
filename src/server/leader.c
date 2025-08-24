#include <stdio.h>
#include <string.h>
#include "server.h"
#include <wait.h>
#include <arpa/inet.h>

int main() {
	initServer();

	// get a server
	struct ServInfo leaderServer;
	if(!getLeader(&leaderServer, 9600)) {
		termServer();
		return 0;
	}

	// start it
	struct ServThread* leaderThread = procLeader(leaderServer);
	if(!leaderThread) {
		termServer();
		return 0;
	}
	char ipAddr[255];
	inet_ntop(AF_INET6, &leaderServer.addr, ipAddr, 255);
	printf("starting leader server at %s\n", ipAddr);

	char buf[255];
	while(fgets(buf, 255, stdin)) {
		*strchrnul(buf, '\n') = '\0';
		if(strncmp(buf, "exit", 255) == 0) {
			break;
		}
		threadMsgSend(leaderThread->coms, buf, 0);
	}

	// free the server and exit
	ServThreadFree(&leaderThread);
	termServer();
	return 0;
}
