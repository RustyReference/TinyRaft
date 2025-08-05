#ifndef RAFT_H
#define RAFT_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    FOLLOWER,
    CANDIDATE,
    LEADER
} State;

typedef struct {
    int id;
    int current_term;
    int voted_for;
    State state;

    int peer_ids[5]; // up to 5 peers
    int num_peers;

    pthread_mutex_t lock;
    pthread_t election_thread;

    uint64_t last_heartbeat_time;
} RaftNode;

// RPC args & reply
typedef struct {
    int term;
    int candidate_id;
} RequestVoteArgs;

typedef struct {
    int term;
    bool vote_granted;
} RequestVoteReply;

void start_election_timer(RaftNode* node);
void handle_request_vote(RaftNode* node, RequestVoteArgs* args, RequestVoteReply* reply);
void start_election(RaftNode* node);
void become_follower(RaftNode* node, int term);

#endif
