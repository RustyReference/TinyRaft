#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "server.h"

struct ServInfo leaderInfo = {0};

// connect to a leader
// @addr : connect at address @addr
// @port : connect to port @port
// #RETURN socket or -1 on error
int connectLeader(char addr[], char port[]);

// handle socket recv for a server
// @leaderSocket : socketfd to recv from 
// #RETURN : NULL on exit
void* handleBackupRecv();

// execute commands for a client server
// @cmd : command in a string 
// @cmdlen : lenght of command
// #RETURN : 0 on exit, -1 on error, 1 on success
int clientExec(char* cmd, int cmdlen);

// connect to a leader and process it until it ultimately falls.
int main([[maybe_unused]]int argc, char *argv[]) {
  // connect to the leader
  int sockfd = connectLeader(argv[1], argv[2]);
  if (sockfd < 0) {
    return 1;
  }

  // connect and start sending stuff
  send(sockfd, "client", 6, 0);

  // start a recv thread for client
  pthread_t tid = 0; 
  pthread_create(&tid, NULL, handleBackupRecv, NULL);
  char buf[1024];
  while(fgets(buf, 1024, stdin)) {
    if(!clientExec(buf, strnlen(buf, 1024))) {
      break;
    }
  }

  // start processing commands
  return 0;
}

// connect to a leader
// @address : connect at address @address
// @port : connect to port @port
// #RETURN socket or -1 on error
int connectLeader(char address[], char port[]) {
  // set options for address
  struct addrinfo addropts = {.ai_family = AF_UNSPEC,     // Both ipv4 and ipv6
                              .ai_socktype = SOCK_STREAM, // Tcp
                              .ai_flags = AI_PASSIVE,     // Wildcard IP binding
                              .ai_protocol = 0,           // Any protocol
                              // uninit
                              .ai_addrlen = 0,
                              .ai_canonname = NULL,
                              .ai_addr = NULL,
                              .ai_next = NULL};
  struct addrinfo *addrlist = NULL;

  // get list of addresses
  int addrerr = getaddrinfo(address, port, &addropts, &addrlist);
  if (addrerr) {
    return -1;
  }

  // go through each address in the list
  struct addrinfo *addr = NULL;
  int sockfd = 0;
  for (addr = addrlist; addr; addr = addr->ai_next) {
    // get a socket
    sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sockfd < 0) {
      continue;
    }
    // connect to it
    if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) != -1) {
      break;
    }
    close(sockfd);
  }

  // check for error
  freeaddrinfo(addrlist);
  if (addr == NULL) {
    return 1;
  }

  // return the socket if success. print beforehand tho
  leaderInfo.addr = *(struct sockaddr_storage*)addr->ai_addr;
  leaderInfo.addrlen = addr->ai_addrlen;
  leaderInfo.sockfd = sockfd;
  return sockfd;
}

// handle socket recv for a server
// @leaderSocket : socketfd to recv from 
// #RETURN : NULL on exit
void* handleBackupRecv() {
  int *oldtype = NULL;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);
  // while recieving communications
  char buf[1024];
  int len = 0;
  while((len = recv(leaderInfo.sockfd, buf, 1024, 0)) > 0) {
    // print them 
    printf("%s\n", buf);
  }  
  pthread_exit(NULL);
}

// execute commands for a client server
// @cmd : command in a string 
// @cmdlen : lenght of command
// #RETURN : 0 on exit, -1 on error, 1 on success
int clientExec(char* cmd, int cmdlen) {
  // sanitize 
  if(!cmd || cmdlen < 0) {
    return -1;
  }

  // split string into command and args
  char *buf[255];
  int arglen = strnsplit(cmd, cmdlen, ' ', buf);
  if(arglen < 0) {
    free(*buf);
    return -1;
  }
  
  // instead of appending args together one by one.
  char* combinedArgs = cmd;
  if(arglen > 1) {
    combinedArgs = strstr(cmd, buf[1]);
  }

  // check exit first
  if(strncmp(buf[0], "exit", cmdlen) == 0) {
    free(*buf);
    return 0;
  }

  // check send next
  if(strncmp(buf[0], "send", cmdlen) == 0) {
    send(leaderInfo.sockfd, combinedArgs, strnlen(combinedArgs, cmdlen), 0);
    free(*buf);
    return 1;
  }

  // print otherwise
  printf("%s\n", combinedArgs);
  free(*buf);
  return -1;
}
