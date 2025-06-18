#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include "node.h"

/**
 * States a server node
 */
int start_node(char *address, int port) {
	int sockmain;
	struct sockaddr_in addr;
	struct in_addr ipLeader;
	
	// Converts the given address to a network-byte-order form
	if(!inet_aton(address, &ipLeader)) {
		fprintf(stderr, "Invalid ip address %s\n", address);
		return 0;
	}

	// Sets up a socket
	sockmain 				= socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family 		= AF_INET;
	addr.sin_port 			= htons(port);
	addr.sin_addr.s_addr 	= ipLeader.s_addr; 
	
	// Connect to leader
	if(connect(sockmain, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Cannot connect to port %d\n", port);
		return 0;
	}

	// success
	return 1;
}

/**
 * Takes the command line option and puts them in the 'peers' attribute
 * of the given Config object
 * @param given the given Config object
 * @param option the command line option to parse
 * @return 0 if success; 1 otherwise
 */
int populate_peers(Config *given, char *option) {
	errno = 0;
	char *dup, *token;

	if ((dup = strdup(option)) == NULL) {
		fprintf(stderr, "populate_peers: Failed to duplicate string: %s",
			strerror(errno));
		
		return 1;
	}

	token = strtok(given, ",");
	while (token != NULL) {
		int npeers = given->num_peers;
		given->peers = realloc(given->peers, sizeof(char *) * (npeers + 1));
		char *given_token = given->peers[npeers] = strdup(token);
		if (given_token == NULL) {
			fprintf(stderr, "populate_peers: Failed to duplicate token: %s",
				strerror(errno));
		}
		
		given->num_peers++;
		token = strtok(NULL, ",");
	}
	
	free(dup);	// Free copy of peer option string
	return 0;
}

/**
 * Takes in the command line options to configure the server node
 * @param argc the number of arguments
 * @param argv the command line arguments, which contain the options
 */
Config parse_args(int argc, char **argv) {
	Config config = {0};
	static struct option long_opts[] = {
		{"id", 		required_argument, 0, 'i'},
		{"port", 	required_argument, 0, 'p'},
		{"peers", 	required_argument, 0, 'x'}
	};

	int code, opt_ind;
	while ((code = getopt_long(argc, argv, "i:p:x:", long_opts, &opt_ind))) {
		switch (code) {
			case 'i':
				config.id = optarg;
				break;
			case 'p':
				config.port = optarg;
				break;
			case 'x':
				populate_peers(&config, optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s --id=<id> --port=<port> --peers=<peers>\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (config.id == 0 || config.port == 0 || config.num_peers == 0) {
		fprintf(stderr, "Missing required arguments.\n");
		exit(EXIT_FAILURE);
	}
	
	return config;
}

/**
 * Deallocates the 'peers' field of the Config object
 */
void free_config_static(Config *given) {
    for (int i = 0; i < given->num_peers; i++) {
        free(given->peers[i]);  // Free each member in the array
    }
    
    free(given->peers);         // Free the array itself
}

/**
 * The main node program that starts the node
 */
int main(int argc, char *argv[]) {
	Config config = parse_args(argc, argv);
	free_config_static(&config);

	return 0;
}