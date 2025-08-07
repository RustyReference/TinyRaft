#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Starts the initial leader node
 * 
 * @param argc the number of command line args
 * @param argv [1]: the port to bind to
 * @return 1 if there is an error
 */
int main(int argc, char* argv[]) {
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("sockfd");
        return 1;
    }

    struct sockaddr_in6 addr = {
        .sin6_family = AF_INET6,
        .sin6_port = htons(9600),
        .sin6_flowinfo = 0,
        .sin6_addr = IN6ADDR_ANY_INIT,
        .sin6_scope_id = 0
	};

    if (argc > 1) {
        // get rid of "bind address already in use "
        int optval = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        // bind
        inet_pton(AF_INET6, argv[1], &addr);

		// Bind to any listening ip address with the same port and info.
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            // error
            perror("bind\n");
            close(sockfd);
            return 1;
        }
    }

    int status = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (status < 0) {
        perror("connect");
    }
    send(sockfd, "backup", sizeof "backup", 0); // Send "backup" to identify this node as backup

    char buf[255] = {0};
    int len = 0;
    while ((len = recv(sockfd, buf, 255, 0)) > 0) {
        printf("%d:%s\n", len, buf);
    }
    close(sockfd);
    return 0;
}
