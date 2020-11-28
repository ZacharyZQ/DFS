#include "dfs_head.h"

void recv_upstream_reply_timeout(struct cycle_s *cycle, struct timer_s *timer,
        void *data)
{
    struct fd_entry_s *fde = (struct fd_entry_s *)data;
    log(LOG_DEBUG, "fd is %d\n", fde->fd);
    cycle_close(cycle, fde);
}

void recv_upstream_reply_callback(cycle_t *cycle, fd_entry_t *fde, void* data) {
    char *buf = calloc(1, 1048576 * 2);
    read(fde->fd, buf, 1048576 * 2);
    log(LOG_DEBUG, "fd is %d, recv message is %s\n", fde->fd, buf);
    cycle_close(cycle, fde);
}

void send_upstream_request_timeout(struct cycle_s *cycle, struct timer_s *timer,
        void *data)
{
    struct fd_entry_s *fde = (struct fd_entry_s *)data;
    log(LOG_DEBUG, "fd is %d\n", fde->fd);
    cycle_close(cycle, fde);
}

void send_upstream_request_callback(struct cycle_s *cycle, struct fd_entry_s *fde,
        ssize_t size, int status, void* data)
{
    if (status != CYCLE_OK) {
        log(LOG_DEBUG, "fd is %d\n", fde->fd);
        cycle_close(cycle, fde);
    }
    cycle_set_timeout(cycle, &fde->timer, 5 * 1000, recv_upstream_reply_timeout, fde);
    cycle_set_event(cycle, fde, CYCLE_READ_EVENT, recv_upstream_reply_callback, NULL);
}

void connect_server_callback(struct cycle_s *cycle, struct fd_entry_s *fde,
        int status, struct in_addr in_addr, int port, void* data)
{
    if (status != CYCLE_OK) {
        log(LOG_DEBUG, "connect error");
        return;
    }
    log(LOG_DEBUG, "connect success fd is %d\n", fde->fd);
    char *buf = strdup("hello, I am slave\n");
    int size = strlen(buf);
    cycle_set_timeout(cycle, &fde->timer, 5 * 1000, send_upstream_request_timeout, fde);
    cycle_write(cycle, fde, buf, size, 2 * 1048576, send_upstream_request_callback, NULL);
}

char hostname[16];
int main () {
    log_init("log/slave.log");
    gethostname(hostname, sizeof(hostname));
    cycle_t *cycle = init_cycle();
    struct in_addr ia;
    fd_entry_t *fde = cycle_open_tcp_nobind(cycle, CYCLE_NONBLOCKING, 0,
            1048576 * 2, 1048576 * 2);
    safe_inet_addr("127.0.0.1", &ia);
    cycle_connect(cycle, fde, ia, 38888, 3, connect_server_callback, NULL);
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
