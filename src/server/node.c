#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include "config.h"
#include "peers.h"
#include "utils.h"

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
 * 
 * Preconditions:
 * 		- 'config' must point to a zero-initialized Config struct
 * 			because num_peers is to start at 0.
 * 			
 * @param config the given Config object
 * @param option the command line option to parse
 * @return 0 if success; 1 otherwise
 */
void populate_peers(Config *config, char *option) {
	errno = 0;
	char *dup, *token;
	dup = strdup_checked(option);

	// Parse peers from arg 'option' to 'config->peers'
	token = strtok(dup, ",");
	while (token != NULL) {
		int npeers = config->num_peers;
		char ip_port[ADDR_LEN] = strndup_checked(token, ADDR_LEN);

		config->peers = (char **) realloc(
			config->peers, sizeof(char *) * (npeers + 1));

		if (ip_port == NULL) {
			fprintf(stderr, 
				"populate_peers(): Failed to duplicate token: %s",
				strerror(errno));
		}
		 
		config->peers[npeers] = strndup_checked(token, ADDR_LEN);
		config->num_peers++;
		token = strtok(NULL, ",");
	}
	
	free(dup);	// Free copy of peer option string
}

/**
 * Takes in the command line options to configure the server node
 * @param config the config object to mutate with the new information
 * @param argc the number of arguments
 * @param argv the command line arguments, which contain the options
 */
void parse_args(Config *config, int argc, char **argv) {
	Config config = {0};
	static struct option long_opts[] = {
		{"id", 		required_argument,	0, 	'i'},
		{"port", 	required_argument, 	0, 	'p'},
		{"peers", 	required_argument, 	0, 	'x'},
		{0, 0, 0, 0}
	};

	int code, opt_ind;
	while (code = getopt_long(argc, argv, "i:p:x:", long_opts, &opt_ind)) {
		switch (code) {
			case 'i':
				config->id = atoi(optarg);
				break;
			case 'p':
				config->port = atoi(optarg);
				break;
			case 'x':
				populate_peers(&config, optarg);
				break;
			case -1: // end of args
				if (config->id == 0 
					|| config->port == 0 
					|| config->num_peers == 0
				) {
					fprintf(stderr, "Missing required arguments.\n");
					exit(EXIT_FAILURE);
				}
				return;
			default:
				fprintf(stderr, 
					"Usage: %s --id=<id> --port=<port> --peers=<peers>\n", 
					argv[0]);
				exit(EXIT_FAILURE);
		}
	}
}

/**
 * Deallocates the 'peers' field of the Config object
 */
void free_config_static(Config *config) {
    for (int i = 0; i < config->num_peers; i++) {
        free(config->peers[i]);  // Free each member in the array
    }
    
    free(config->peers);         // Free the array itself
}

void test() {
	char buff[21];
	int port;
	char *addr = "127.0.0.1:3000";
	sscanf(addr, "%[^:]:%d", buff, &port);
	printf("The match is: %s and the port is: %d\n", buff, port);
}

/**
 * The main node program
 */
int main(int argc, char *argv[]) {
	Config config = {0};
	Peer_arr *peers;
	parse_args(&config, argc, argv);	
	peers = get_peers(&config);			// Must free at the end
	free_config_static(&config);


	/*
	test();
	*/

	return 0;
}
