// raft_simulation.c - Simplified Raft simulation in C11
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define NUM_NODES 5
#define ELECTION_TIMEOUT_MIN 150
#define ELECTION_TIMEOUT_MAX 300
#define HEARTBEAT_INTERVAL 50

typedef enum { FOLLOWER, CANDIDATE, LEADER } Role;

typedef struct {
    int id;
    Role role;
    int term;
    int votedFor;
    int electionTimer;
    int heartbeatTimer;
    int votesReceived;
    bool alive;
} Node;

Node nodes[NUM_NODES];
int currentTime = 0;

int random_timeout() {
    return ELECTION_TIMEOUT_MIN + rand() % (ELECTION_TIMEOUT_MAX - ELECTION_TIMEOUT_MIN);
}

void reset_node(Node* node) {
    node->role = FOLLOWER;
    node->term = 0;
    node->votedFor = -1;
    node->electionTimer = random_timeout();
    node->heartbeatTimer = HEARTBEAT_INTERVAL;
    node->votesReceived = 0;
    node->alive = true;
}

void init_nodes() {
    for (int i = 0; i < NUM_NODES; i++) {
        nodes[i].id = i;
        reset_node(&nodes[i]);
    }
}

void send_heartbeat(int leaderId) {
    for (int i = 0; i < NUM_NODES; i++) {
        if (i != leaderId && nodes[i].alive) {
            nodes[i].electionTimer = random_timeout();
            nodes[i].role = FOLLOWER;
            nodes[i].term = nodes[leaderId].term;
            nodes[i].votedFor = leaderId;
            printf("Leader %d -> Follower %d: Heartbeat\n", leaderId, i);
        }
    }
}

void start_election(int candidateId) {
    nodes[candidateId].role = CANDIDATE;
    nodes[candidateId].term += 1;
    nodes[candidateId].votedFor = candidateId;
    nodes[candidateId].votesReceived = 1; // vote for self
    nodes[candidateId].electionTimer = random_timeout();
    printf("Node %d starts election for term %d\n", candidateId, nodes[candidateId].term);

    for (int i = 0; i < NUM_NODES; i++) {
        if (i != candidateId && nodes[i].alive &&
            (nodes[i].votedFor == -1 || nodes[i].votedFor == candidateId)) {
            nodes[i].votedFor = candidateId;
            nodes[i].term = nodes[candidateId].term;
            nodes[candidateId].votesReceived++;
            printf("Node %d votes for Node %d\n", i, candidateId);
        }
    }

    if (nodes[candidateId].votesReceived > NUM_NODES / 2) {
        nodes[candidateId].role = LEADER;
        nodes[candidateId].heartbeatTimer = HEARTBEAT_INTERVAL;
        printf("Node %d becomes Leader for term %d\n", candidateId, nodes[candidateId].term);
    }
}

void tick_node(Node* node) {
    if (!node->alive) return;

    node->electionTimer--;

    if (node->role == LEADER) {
        node->heartbeatTimer--;
        if (node->heartbeatTimer <= 0) {
            send_heartbeat(node->id);
            node->heartbeatTimer = HEARTBEAT_INTERVAL;
        }
    } else {
        if (node->electionTimer <= 0) {
            start_election(node->id);
        }
    }
}

void simulate_step() {
    for (int i = 0; i < NUM_NODES; i++) {
        tick_node(&nodes[i]);
    }
    currentTime += 1;
}

void simulate(int steps) {
    for (int t = 0; t < steps; t++) {
        simulate_step();
    }
}

const char* role_to_str(Role r) {
    switch (r) {
        case FOLLOWER: return "Follower";
        case CANDIDATE: return "Candidate";
        case LEADER: return "Leader";
        default: return "Unknown";
    }
}

void print_node_states() {
    printf("\nTime %d - Node States:\n", currentTime);
    for (int i = 0; i < NUM_NODES; i++) {
        printf("Node %d: %s | Term: %d | VotedFor: %d | ElectionTimer: %d\n",
               nodes[i].id, role_to_str(nodes[i].role), nodes[i].term,
               nodes[i].votedFor, nodes[i].electionTimer);
    }
}

int main() {
    srand(time(NULL));
    init_nodes();

    for (int i = 0; i < 1000; i++) {
        simulate_step();
        if (i % 50 == 0) {
            print_node_states();
        }
    }

    return 0;
}
