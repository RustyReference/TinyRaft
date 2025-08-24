#define _GNU_SOURCE
#include "server.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <wait.h>

void printBackups();

int main() {
  initServer();

  // get a server
  struct ServInfo leaderServer;
  if (!getLeader(&leaderServer, "9600")) {
    termServer();
    return 0;
  }

  // start it
  struct ServThread *leaderThread = procLeader(leaderServer);
  if (!leaderThread) {
    termServer();
    return 0;
  }
  
  // TODO: Make it a real function instead of system() call.
  printf("starting server at ");
  fflush(STDIN_FILENO);
  system("hostname -I");

  char buf[255];
  while (fgets(buf, 255, stdin)) {
    *strchrnul(buf, '\n') = '\0';
    if (strncmp(buf, "exit", 255) == 0) {
      break;
    }
    threadMsgSend(leaderThread->coms, buf, 0);
  }

  // free the server and exit
  ServThreadFree(&leaderThread);
  termServer();
  return 0;
}
