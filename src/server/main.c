#include <stdio.h>
#include <string.h>
#include "server.h"
#include "leader.h"
#include <wait.h>


int main() {
	initServer();

	struct ServInfo info;
	getLeader(&info, 9600);

	struct ServThread* leaderThread = procLeader(info);
	char buf[255];
	while(fgets(buf, 255, stdin)) {
		buf[strnlen(buf, 255)-1] = '\0';
		threadMsgSend(leaderThread->coms, buf, 0);
		if(strncmp(buf, "exit", 255) == 0) {
			break;
		}
	}
	ServThreadFree(&leaderThread);
	
	termServer();
	return 0;
}
