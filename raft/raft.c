#include "raft.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <linux/time.h>
#define _POSIX_C_SOURCE 200809L

uint64_t current_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

void become_follower(RaftNode* node, int term) {
    node->state = FOLLOWER;
    node->current_term = term;
    node->voted_for = -1;
    node->last_heartbeat_time = current_millis();
    printf("Node %d becomes FOLLOWER (term %d)\n", node->id, term);
}

void* election_timer_thread(void* arg) {
    RaftNode* node = (RaftNode*)arg;

    while (1) {
        usleep(100 * 1000);  // check every 100ms
        pthread_mutex_lock(&node->lock);

        uint64_t now = current_millis();
        if (node->state != LEADER &&
            now - node->last_heartbeat_time > (150 + rand() % 150)) {
            printf("Node %d election timeout, starting election\n", node->id);
            start_election(node);
        }

        pthread_mutex_unlock(&node->lock);
    }
    return NULL;
}

void start_election_timer(RaftNode* node) {
    pthread_create(&node->election_thread, NULL, election_timer_thread, node);
}

void start_election(RaftNode* node) {
    node->state = CANDIDATE;
    node->current_term += 1;
    node->voted_for = node->id;
    node->last_heartbeat_time = current_millis();

    printf("Node %d becomes CANDIDATE (term %d), votes for self\n",
           node->id, node->current_term);

    int votes = 1;  // vote for self

    for (int i = 0; i < node->num_peers; i++) {
        int peer_id = node->peer_ids[i];

        RequestVoteArgs args = {node->current_term, node->id};
        RequestVoteReply reply;

        // Fake peer call (stub)
        handle_request_vote(node, &args, &reply);

        if (reply.vote_granted) {
            votes++;
        }
    }

    if (votes > (node->num_peers + 1) / 2) {
        node->state = LEADER;
        printf("Node %d becomes LEADER!\n", node->id);
    } else {
        printf("Node %d did not get majority\n", node->id);
    }
}

void handle_request_vote(RaftNode* node, RequestVoteArgs* args, RequestVoteReply* reply) {
    if (args->term > node->current_term) {
        become_follower(node, args->term);
    }

    reply->term = node->current_term;
    if (args->term == node->current_term && (node->voted_for == -1 || node->voted_for == args->candidate_id)) {
        node->voted_for = args->candidate_id;
        node->last_heartbeat_time = current_millis();
        reply->vote_granted = true;
        printf("Node %d voted for %d\n", node->id, args->candidate_id);
    } else {
        reply->vote_granted = false;
    }
}
