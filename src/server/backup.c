#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main() {
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
	int status = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if(status < 0) {
		perror("connect");
	}
	sleep(5);
	return 0;
	//send(sockfd, "backup", sizeof "backup", 0);

	char buf[255];
	int len;
	while((len = recv(sockfd, buf, 255, 0)) > 0) {
		buf[len] = '\0';
		printf("%s\n", buf);
		break;
	}
	return 0;
}
