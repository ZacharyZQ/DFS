#include "dfs_head.h"
int main () {
    log_init("log/slave.log");
    log(LOG_RUN_ERROR, "slave start\n");
    cycle_t *cycle = init_cycle();
    struct in_addr ia; 
    ia.s_addr = INADDR_ANY;
    int listen_fd = open_listen_fd(
            CYCLE_NONBLOCKING | CYCLE_REUSEADDR, ia, 48888, 1); 
    fd_entry_t *listen_fde = cycle_create_listen_fde(cycle, listen_fd);
    cycle_listen(cycle, listen_fde);
    cycle_set_timeout(cycle, &listen_fde->timer, 1000, accept_client_timeout, NULL);
    cycle_set_event(cycle, listen_fde, CYCLE_READ_EVENT, accept_client_handler, NULL);
    slave_register(cycle);
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
