#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char* argv[]) {
	int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("sockfd");
	}

	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(9600),
		.sin6_flowinfo = 0,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_scope_id = 0
	};
	if(argc > 1) {
		inet_pton(AF_INET6, argv[1], &addr);
	}
	int status = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if(status < 0) {
		perror("connect");
	}
	send(sockfd, "backup", sizeof "backup", 0);

	char buf[255] = { 0 };
	int len = 0;
	while((len = recv(sockfd, buf, 255, 0)) > 0) {
		printf("%d:%s\n", len, buf);
	}
	close(sockfd);
	return 0;
}
