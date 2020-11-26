#ifndef __EPOLL_H
#define __EPOLL_H
#define MAX_EVENTS 256
#include "list.h"
struct epoll_s;
struct cycle_s;
struct fd_entry_s;

typedef struct epoll_s {
    int epoll_fd;
    int num_fds;
    unsigned int *old_fd_events;
    struct cycle_s *cycle;
} epoll_t;

epoll_t* epoll_event_driven_init();
void epoll_event_driven_destroy(epoll_t *e);
void epoll_set_event(epoll_t *e, struct fd_entry_s* fde,
       int read, int write);
int epoll_event_driven_wait(epoll_t *e, int loopdelay);
int epoll_event_driven_wait_post(epoll_t *e, int loopdelay,
        struct list_head* accept_events, struct list_head* normal_events);
void epoll_event_driven_close(epoll_t *e, struct fd_entry_s* fde);
void epoll_event_driven_shutdown(epoll_t *e);

#endif
