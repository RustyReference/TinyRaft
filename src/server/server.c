#ifndef LEADER_H
#define LEADER_H

#include "server.h"
#include <arpa/inet.h>
#include <sys/time.h>

#ifndef MAXID
#define MAXID 16
#endif

// A pool of unique ids.
struct {
    int ids[MAXID];
    pthread_mutex_t lock;
} idpool = {0};

// list of servers
struct ServListSafe clientList, backupList;
struct ServThread* leader = NULL;

// init all startup stuff and global variables.
void initServer(void) {
    // server list
    SLIST_INIT(&clientList.servers);
    pthread_mutex_init(&clientList.lock, NULL);
    SLIST_INIT(&backupList.servers);
    pthread_mutex_init(&backupList.lock, NULL);

    // id pool
    memset(idpool.ids, 0, sizeof idpool.ids);
    pthread_mutex_init(&idpool.lock, NULL);
}

// terminate all startup stuff.
void termServer(void) {
    // clientlist
    pthread_mutex_lock(&clientList.lock);
    struct ServListEntry* np = SLIST_FIRST(&clientList.servers);
    while (np) {
        struct ServListEntry* temp = SLIST_NEXT(np, servers);
        ServThreadFree(&np->server);
        free(np);
        np = temp;
    }

    // backuplist
    pthread_mutex_lock(&backupList.lock);
    np = SLIST_FIRST(&backupList.servers);
    while (np) {
        struct ServListEntry* temp = SLIST_NEXT(np, servers);
        // free(np);
        ServThreadFree(&np->server);
        free(np);
        np = temp;
    }

    pthread_mutex_destroy(&backupList.lock);
    pthread_mutex_unlock(&backupList.lock);

    // idpool
    pthread_mutex_destroy(&idpool.lock);
}

// get first available id.
int getId() {
    int res = -1;
    pthread_mutex_lock(&idpool.lock);
    // get first index with zero-value
    for (int i = 0; i < MAXID; i++) {
        if (!idpool.ids[i]) {
            idpool.ids[i] = 1;
            res = i;
            break;
        }
    }
    pthread_mutex_unlock(&idpool.lock);
    // i on success and -1 on error.
    return res;
}

// free @id back to the pool.
void clearId(int id) {
    pthread_mutex_lock(&idpool.lock);
    idpool.ids[id] = 0;
    pthread_mutex_unlock(&idpool.lock);
}

// Init a leader server.
// @dst : Store the socket and address in @dst
// @port : Bind to port @port
// #RETURN : 0 on error. 1 on success.
int getLeader(struct ServInfo* dst, int port) {
    errno = 0;

    // get a socket sockfd
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
        dst = NULL;
        return 0;
    }

    // get rid of the pesky "bind address already in use" message.
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // init info
    if (!dst) {
        errno = EDESTADDRREQ;
        dst = NULL;
        return 0;
    }

    // set address
    dst->addr = (struct sockaddr_in6){
        .sin6_family = AF_INET6,
        .sin6_port = htons(port),
        .sin6_flowinfo = 0,
        .sin6_addr = IN6ADDR_ANY_INIT,
        .sin6_scope_id = 0};
    dst->addrlen = sizeof(dst->addr);

    // bind address to socket sockfd
    if (bind(sockfd, (struct sockaddr*)&dst->addr, dst->addrlen) < 0) {
        dst = NULL;
        return 0;
    }

    // start listening on address
    if (listen(sockfd, 5) < 0) {
        dst = NULL;
        return 0;
    }
    dst->sockfd = sockfd;
    return 1;
}

// process commands in a thread for some leader server
// @server : ServThread to use. Look at procLeader for example process.
// #RETURN : NULL on exit.
void* leaderCommandThread(void* leaderServer) {
    int* oldtype = NULL;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);
    // sanitize
    if (!leaderServer) {
        pthread_exit(NULL);
    }
    // init
    struct ServThread* server = (struct ServThread*)leaderServer;
    leader = server;
    struct ThreadMsg* coms = server->coms;

    // while getting messages
    char* buf = NULL;
    int len = 0;
    while ((len = threadMsgRecv(coms, &buf)) >= 0) {
        if (!leaderCommandExec(buf, len)) {
            break;
        }
        free(buf);
        buf = NULL;
    }
    free(buf);
    pthread_exit(NULL);
}

// start processing a leader server
// @info : get socket from @info.
// @RETURN : ThreadMsg. NULL on error.
// 	tid[0] is used to process commands.
// 	tid[1] is used to accept new connections.
struct ServThread* procLeader(struct ServInfo info) {
    // get an id for the leader.
    int id = getId();
    if (id < 0) {
        // ...HOW??
        return NULL;
    }

    // setup commands
    struct ThreadMsg* coms = ThreadMsgCreat();
    if (!coms) {
        // abort
        clearId(id);
        return NULL;
    }

    // get result
    struct ServThread* server = (struct ServThread *)malloc(sizeof(struct ServThread));
    if (!server) {
        // abort
        clearId(id);
        ThreadMsgFree(&coms);
        return NULL;
    }

    // set result
    *server = (struct ServThread) {
        .info = info,
        .coms = coms,
        .tid = (pthread_t*)calloc(2, sizeof(pthread_t)),
        .tlen = 2, // 1 for commands and 1 more for good measure
        .id = id
    };

    // start the threads
    if (!server->tid) {
        // abort
        clearId(id);
        ThreadMsgFree(&coms);
        free(server);
        return NULL;
    }

    // start the threads
    if (pthread_create(&server->tid[0], NULL, leaderCommandThread, server)) {
        // abort
        clearId(id);
        ThreadMsgFree(&coms);
        free(server->tid);
        free(server);
        return NULL;
    }

    // DRY stands for "do repeat yourself"
    if (pthread_create(&server->tid[1], NULL, leaderAcceptThread, server)) {
        // abort
        clearId(id);
        ThreadMsgFree(&coms);
        free(server->tid);
        free(server);
        return NULL;
    }

    leader = server;
    return leader;
}

// free ServThread the pointer @server from memory
void ServThreadFree(struct ServThread** server) {
    struct ServThread* contents = *server;
    struct ThreadMsg* coms = contents->coms;

    int sockfd = contents->info.sockfd;

    // free stored pointers
    close(sockfd);
    clearId(contents->id);
    // pthread_mutex_lock(&coms->mlock);

    // cancel threads
    for (int i = 0; i < contents->tlen; i++) {
        pthread_cancel(contents->tid[i]);
        pthread_join(contents->tid[i], NULL);
    }

    // free mutex
    //	pthread_mutex_unlock(&coms->mlock);
    pthread_mutex_destroy(&coms->mlock); // for the freebsd users, all 2 of them.
    ThreadMsgFree(&coms);

    // free structs
    free(contents->tid);
    free(contents);
    *server = NULL;
}

// split a string into multiple strings
// @str : string you wish to split.
// @len : max lenght of the str
// @delim : delimiter to use when splitting.
// @buf : Will store the strings in this array.
// #RETURN : lenght of array
// 	-1 on error
// #NOTES :
// 	free *buf once without looping over once done, ex.
// 	char* buf[255];
// 	int len = strnsplit("hello there how is it going", 1024, ' ', buf);
// 	if(len > 0) {
// 		...
// 		free(*buf);
// 	}
int strnsplit(char* str, int len, char delim, char* buf[]) {
    if (!str) {
        return -1;
    }

    // skip start
    char* cpy = strndup(str, len);
    while (cpy[0] == delim) {
        cpy++;
    }
    buf[0] = cpy;

    // go around seperating stuff.
    int i;
    for (i = 1; *(cpy = strchrnul(cpy, delim)); i++) {
        // skip through delims
        while (*cpy == delim) {
            *cpy = '\0';
            cpy++;
        }
        // set element
        buf[i] = cpy;
    }
    buf[i + 1] = NULL;

    // free copy
    return i;
}

// TODO: DELETE AFTER DEMO
//  write foo = goo to db.
char fooList[255][25] = {0};
char gooList[255][25] = {0};
void dbwrite(char foo[], char goo[]) {
    // select valid foo
    int i = 0;
    while (i < 255) {
        if (strncmp(fooList[i], "", 1) == 0 || strncmp(fooList[i], foo, 25) == 0) {
            break;
        }
        i++;
    }
    if (i > 255) {
        return;
    }
    strncpy(fooList[i], foo, 25);
    strncpy(gooList[i], goo, 25);
}

int dbRead(char foo[], char buf[]) {
    for (int i = 0; i < 255; i++) {
        if (strncmp(fooList[i], foo, 25) == 0) {
            strncpy(buf, gooList[i], 25);
            return 1;
        }
    }
    return 0;
}

// execute a command for the leader
// @cmd : Command
// @cmdlen : maxsize of the command
// #RETURN : negative on error, otherwise ID of the command.
// 	-1 : invalid/error
// 	0 : exit command
// 	1 : backup-all
// 	2 : client-all
// TODO: decide if the leader node will ever need to echo messages to each follower without actually operating.
// TODO: implement looking at second token of cmd to see if it is "write" or "delete"
int leaderCommandExec(char* cmd, int cmdlen) {
    // sanitize
    if (!cmd || cmdlen <= 0) {
        return -1;
    }

    // pretty well optimized so might as well
    char* buf[255];
    int arglen = strnsplit(cmd, cmdlen, ' ', buf);
    if (arglen < 0) {
        free(*buf);
        return -1;
    }
    int firstlen = strnlen(buf[0], cmdlen) + 1;

    // redirections
    cmd += firstlen;

    // exit
    if (strncmp(buf[0], "exit", firstlen) == 0) {
        free(*buf);
        return 0;
    }

    // backup-all
    /* TODO: try to understand this. */
    if (strncmp(buf[0], "backup-all", firstlen) == 0) {
        broadcastMsg(backupList, cmd, 0);
        leaderCommandExec(cmd, strnlen(cmd, cmdlen) + 1); // recursion at its finest!
        free(*buf);

        return 1;
    }

    // client-all
    if (strncmp(buf[0], "client-all", firstlen) == 0) {
        broadcastMsg(clientList, cmd, 0);
        free(*buf);

        return 2;
    }

    // TODO: DELETE AFTER DEMO
    if (strncmp(buf[0], "write", firstlen) == 0) { // example: backup-all write x = 3
        dbwrite(buf[1], buf[3]); // Pass in the key and value: (e.g. write key = value)
        broadcastMsg(backupList, cmd, 0);
        free(*buf);

        return 5;
    } else if (strncmp(buf[0], "read", firstlen) == 0) {
        char dbuf[25] = {0};
        if (dbRead(buf[1], dbuf)) {
            broadcastMsg(clientList, dbuf, 0);
        } else {
            printf("%s\n", "No value found.");
        }
        free(*buf);

        return 4;
    }

    // end of redirections.
    cmd -= firstlen;
    printf("%s\n", cmd); // Echo to stdout for testing purposes.

    free(*buf);
    return -1;
}

// broadcast a message to a list of servers
// @server : list of servers
// @msg : what to say
// @maxlen : size of the message, set <= 0 for strnlen
void broadcastMsg(struct ServListSafe serverlist, char* msg, int maxlen) {
    // sanitize
    if (!msg) {
        return;
    }
    if (maxlen < 0) {
        maxlen = 0;
    }

    // loop through while sending
    pthread_mutex_lock(&serverlist.lock);
    struct ServListEntry* current;
    // whoever made SLIST has my respect for this syntax
    SLIST_FOREACH(current, &serverlist.servers, servers) {
        threadMsgSend(current->server->coms, msg, 0);
    }
    // end
    pthread_mutex_unlock(&backupList.lock);
}

// Accept a new server and start its own thread
// @leaderServer : ServThread of the leaderServer.
// #RETURN : NULL on exit.
void* leaderAcceptThread(void* leaderServer) {
    // init
    int* oldtype = NULL;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);
    struct ServThread* server = (struct ServThread*)leaderServer;

    // init bufs
    socklen_t addrlen = 0;
    struct sockaddr_in6 addr = {0};
    int sockfd;

    // For each new connection.
    while ((sockfd = accept(server->info.sockfd, (struct sockaddr*)&addr, &addrlen)) > 0) {
        // Gather the information
        struct ServInfo* info = malloc(sizeof(struct ServInfo));
        *info = (struct ServInfo){
            .sockfd = sockfd,
            .addr = addr,
            .addrlen = addrlen};

        // Start a new thread with it.
        pthread_t tid;
        pthread_create(&tid, NULL, leaderAddServer, info);
        pthread_detach(tid);
    }

    pthread_exit(NULL);
}

// add a server and start processing it based on backup or client
// @servThread : pointer to the ServThread struct with info and tid setup before
// #RETURN : NULL
void* leaderAddServer(void* servInfo) {
    int* oldtype = NULL;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);

    // init
    struct ServThread* servThread = malloc(sizeof(struct ServThread));
    servThread->info = *(struct ServInfo*)servInfo;
    int sockfd = servThread->info.sockfd;
    free(servInfo);

    // timeout for recv
    const struct timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // get first message to see if valid
    char firstMsg[255];
    int status = recv(sockfd, firstMsg, 255, 0);
    if (status <= 0) {
        // timeout
        free(servThread);
        pthread_exit(NULL);
    }

    if (strncmp(firstMsg, "backup", 255) == 0) {
        // add to backupList and process it
        backupRecv(servThread);
    }

    if (strncmp(firstMsg, "client", 255) == 0) {
        // add to backupList and process it
        clientRecv(servThread);
    }
    pthread_exit(NULL);
}

// Recieve communications from a command to process backupThread.
// Will automatically free backupThread once backup disconnects, error or otherwise.
// @backupThread : ServThread with info ready. Will steal the pointer.
// #NOTES
// 	Steals the one and only pointer, do not free anything.
void backupRecv(struct ServThread* backupThread) {
    struct ServListEntry* np = addServListSafe(&backupList, backupThread);
    // set an id and start command thread
    backupThread->id = getId();
    backupThread->tid = calloc(2, sizeof(pthread_t));
    pthread_create(&backupThread->tid[0], NULL, backupCommandThread, backupThread);
    backupThread->tid[1] = pthread_self();
    backupThread->tlen = 2;

    // set coms
    backupThread->coms = ThreadMsgCreat();

    // no timeout for recv
    int sockfd = backupThread->info.sockfd;
    const struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // start recieving requests
    char buf[1024];
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (recv(sockfd, buf, 1024, 0) > 0) {
        *strchrnul(buf, '\n') = '\0';
        // TODO: backupCommandExec
    }

    // Remove from list and exit
    pthread_mutex_lock(&backupList.lock);
    SLIST_REMOVE(&backupList.servers, np, ServListEntry, servers);
    free(np);
    pthread_mutex_unlock(&backupList.lock);
    backupThread->tlen--;
    ServThreadFree(&backupThread);
    pthread_exit(NULL);
}

// Start processing commands for a backupThread in their own thread.
// @backupThread : ServThread fully setup and ready.
// #RETURN : NULL
// NOTES
// 	Communicate while running using the coms inside backupThread.
void* backupCommandThread(void* backupThread) {
    int* oldtype = NULL;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);

    // init
    struct ServThread* thread = backupThread;
    struct ThreadMsg* coms = thread->coms;
    char* buf = NULL;
    int len = 0;
    while ((len = threadMsgRecv(coms, &buf) >= 0)) {
        // just necessary for backupCommandThread only for some reason.
        send(thread->info.sockfd, buf, strnlen(buf, 1024) + 1, 0);
        free(buf);
    }
    thread->tlen--;
    thread->tid[0] = thread->tid[1]; // ?
    pthread_exit(NULL);
}

// Add servThread to servList.
// @servList : clientList or backupList.
// @servThread : the thread complete with tid and all.
// #RETURN : NULL on error and entry on success
// NOTES
// 	This function steals the servThread pointer. You may not free it anymore.
// 	DO NOT FREE SERVTHREAD AFTER ITS BEEN ADDED WITHOUT REMOVING FIRST.
struct ServListEntry* addServListSafe(struct ServListSafe* servList, struct ServThread* servThread) {
    // sanitize
    if (!servList || !servThread) {
        return NULL;
    }

    // init
    pthread_mutex_lock(&servList->lock);
    struct ServListEntry* entry = malloc(sizeof(struct ServListEntry));
    entry->server = servThread;
    SLIST_INSERT_HEAD(&servList->servers, entry, servers);
    pthread_mutex_unlock(&servList->lock);

    // exit
    return entry;
}

// Recieve communications from a command to process clientThread.
// Will automatically free clientThread once backup disconnects, error or otherwise.
// @clientThread : ServThread with info ready. Will steal the pointer.
// #NOTES
// 	Steals the one and only pointer, do not free anything.
void clientRecv(struct ServThread* clientThread) {
    struct ServListEntry* np = addServListSafe(&clientList, clientThread);
    // set an id and start command thread
    clientThread->id = getId();
    clientThread->tid = calloc(2, sizeof(pthread_t));
    pthread_create(&clientThread->tid[0], NULL, clientCommandThread, clientThread);
    clientThread->tid[1] = pthread_self();
    clientThread->tlen = 2;

    // set coms
    clientThread->coms = ThreadMsgCreat();

    // no timeout for recv
    int sockfd = clientThread->info.sockfd;
    const struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // start recieving requests
    char buf[1024];
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (recv(sockfd, buf, 1024, 0) > 0) {
        *strchrnul(buf, '\n') = '\0';
        // TODO: clinetCommandExec
        // printf("client at %s says %s\n", "address", buf);
        threadMsgSend(leader->coms, buf, 0);
    }

    // Remove from list and exit
    pthread_mutex_lock(&clientList.lock);
    SLIST_REMOVE(&clientList.servers, np, ServListEntry, servers);
    pthread_mutex_unlock(&clientList.lock);
    ServThreadFree(&clientThread);
    clientThread->tlen--;
    pthread_exit(NULL);
}

// Start processing commands for a clientThread in their own thread.
// @clientThread : ServThread fully setup and ready.
// #RETURN : NULL
// NOTES
// 	Communicate while running using the coms inside clientThread.
void* clientCommandThread(void* clientThread) {
    int* oldtype = NULL;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldtype);

    // init
    struct ServThread* thread = clientThread;
    struct ThreadMsg* coms = thread->coms;
    char* buf = NULL;
    int len = 0;
    while ((len = threadMsgRecv(coms, &buf) >= 0)) {
        // just necessary for clientCommandThread only for some reason.
        send(thread->info.sockfd, buf, strlen(buf) + 1, 0);
        // send(thread->info.sockfd, buf, strnlen(buf, 1024)+1, 0);
        free(buf);
    }
    thread->tlen--;
    thread->tid[0] = thread->tid[1]; // ?
    pthread_exit(NULL);
}

#endif
