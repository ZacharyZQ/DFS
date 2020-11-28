#include "dfs_head.h"
char hostname[16];
int main () {
    log_init("log/slave.log");
    gethostname(hostname, sizeof(hostname));
    cycle_t *cycle = init_cycle();
    while (1) {
        time_update();
        LIST_HEAD(accept_events);
        LIST_HEAD(normal_events);
        cycle_wait_post_event(cycle, &accept_events, &normal_events);
        process_posted_events(cycle, &accept_events);
        process_posted_events(cycle, &normal_events);
        cycle_check_timeouts(cycle);
    }
    return 0;
}
