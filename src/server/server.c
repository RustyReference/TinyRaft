
#include "server.h"

/* For backup servers */

// Connect to a leader.
int connectLeader(char* addr, int port);


/* For leader servers */

// send changes to backup servers
int giveChanges(char* key, char* value);

// send logs to backup servers
int giveLogs(char* filepath);

// send database to backup servers
int givedb(char* filepath);


/* For both */ 

// init and start the server.
int initServer(int port, int leader, char pathLogs[255], char pathdb[255], char passwd[255]) {
	
	// defaults 
	isLeader = leader;
	if(!pathLogs[0]) { strncpy(pathLogs, "./files/logs", 255); }
	if(!pathdb[0]) { strncpy(pathdb, "./files/pathdb", 255); }
	if(!passwd[0]) { strncpy(passwd, "burenyuu", 255); }
	int status = 0;

	// currently working on here

	return 1;
}

// write to db
int writedb(char* key, char* value);
