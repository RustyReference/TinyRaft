#include "../include.h"

int main() {
	// start the server
	struct servInfo leaderServer;
	if(!startServer(&leaderServer, 9600)) {
		perror("start server\n");
		return 1;
	}

	// make it leader for now
	int status = processLeader(leaderServer, "1234", "1234");
	if(status) {
		close(leaderServer.sockfd);
	} 

	// yippeee
	return !status;
}
