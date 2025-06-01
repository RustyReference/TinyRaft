#include "../include.h"

int main(int argc, char* argv[]) {
	int sockserver = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(9600);

	int status = connect(sockserver, (struct sockaddr*)&address, sizeof(address));
	if(status == -1) {
		printf("Error\n");
	} else {
		char buf[255];
		while(fgets(buf, 255, stdin)) {
			buf[strnlen(buf, 255)-1] = '\0';
			send(sockserver, buf, 255, 0);
			if(strncmp(buf, "exit", 255) == 0) {
				break;
			}
		}
	}

	return 0;
}

