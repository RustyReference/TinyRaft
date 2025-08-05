#include "raft.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main() {
    srand(time(NULL));

    RaftNode node = {
        .id = 1,
        .state = FOLLOWER,
        .current_term = 0,
        .voted_for = -1,
        .num_peers = 0,
        .last_heartbeat_time = 0
    };
    pthread_mutex_init(&node.lock, NULL);

    start_election_timer(&node);

    // Let the simulation run
    while (1) {
        sleep(1);
    }

    return 0;
}
