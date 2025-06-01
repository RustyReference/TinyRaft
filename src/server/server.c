
#include "server.h"

/* For backup servers */

// start the backup server
int startBackup(char* address, int port) {
	struct sockaddr_in addr;
	struct in_addr ipLeader;
	
	// Converts the given address to a network-byte-order form
	if(!inet_aton(address, &ipLeader)) {
		fprintf(stderr, "Invalid ip address %s\n", address);
		return 1;
	}

	// Sets up a socket
	sockmain 				= socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family 		= AF_INET;
	addr.sin_port 			= htons(port);
	addr.sin_addr.s_addr 	= ipLeader.s_addr; 
	
	// Connect to leader
	if(connect(sockmain, (struct sockaddr*)&addr, sizeof(addr))) {
		fprintf(stderr, "Cannot connect to port %d\n", port);
		return 0;
	}

	// success
	return 1;
}

// Connect to a leader.
int connectLeader(char* addr, int port);


/* For leader servers */

// start leader server
int startLeader(int port) {
	// setup the main socket
	sockself = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in* addr;
	addr->sin_family = AF_INET;
	addr->sin_port = port;
	addr->sin_addr.s_addr = INADDR_ANY;

	// start listening
	if(bind(sockself, (struct sockaddr*)addr, sizeof(*addr))) { return 0; }
	if(listen(sockself, 2)) { return 0; }
	
	// success
	return 1;
}

// send changes to backup servers
int giveChanges(char* key, char* value);

// send logs to backup servers
int giveLogs(char* filepath);

// send database to backup servers
int givedb(char* filepath);


/* For both */ 

// init and start the server.
int initServer(int port, int leader, char pathLogs[255], char pathdb[255], char passwd[255]) {
	
	// set defaults 
	if(!pathLogs[0]) { strncpy(pathLogs, "./files/logs", 255); }
	if(!pathdb[0]) { strncpy(pathdb, "./files/pathdb", 255); }
	if(!passwd[0]) { strncpy(passwd, "burenyuu", 255); }
	int status = 0;

	// open the files and check for permissions.
	logs = fopen(pathLogs, "w+");
	if(!logs) { 
		fprintf(stderr, "Can't write to file %s\n", pathLogs); 
		return 0;
	}
	db = fopen(pathdb, "a+");
	if(!db) { 
		fprintf(stderr, "Can't read the database at %s\n", pathdb); 
		return 0;
	}

	// start the server
	int serverStatus = 0;
	if (leader) {
		status = startLeader(port);
	} else {
		status = startBackup(port);
	}
	if(!serverStatus) {
		fprintf(stderr, "There was a problem starting the server at port %d\n", port);
		return 0;
	}

	// started the server yippeee
	return 1;
}

// write to db
int writedb(char* key, char* value);

