#include "dfs_head.h"
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include "epoll.h"
#include "cycle.h"
epoll_t* epoll_event_driven_init() {
    int epollfd = epoll_create(1);

    if (epollfd < 0) {
        assert(0);
    }
    set_close_on_exec(epollfd);
    epoll_t* e = (epoll_t*)calloc(1, sizeof(epoll_t));
    e->num_fds = 0;
    e->epoll_fd = epollfd;
    e->old_fd_events = (unsigned int*)calloc(65536, sizeof(unsigned int));

    return e;
}

void epoll_event_driven_destroy(epoll_t *e) {
    close(e->epoll_fd);
    free(e);
}

void epoll_set_event(epoll_t *e, fd_entry_t* fde,
       int read, int write)
{
    struct epoll_event ev;
    ev.events = 0;

    if (read) {
        ev.events |= EPOLLIN;
    }

    if (write) {
        ev.events |= EPOLLOUT;
    }

    if (ev.events) {
        ev.events |= EPOLLHUP | EPOLLERR;
    }

    if (ev.events != e->old_fd_events[fde->fd]) {
        int epoll_ctl_type;

        if (!ev.events) {
            epoll_ctl_type = EPOLL_CTL_DEL;
        } else if (e->old_fd_events[fde->fd]) {
            epoll_ctl_type = EPOLL_CTL_MOD;
        } else {
            epoll_ctl_type = EPOLL_CTL_ADD;
        }

        ev.data.ptr = fde;
        e->old_fd_events[fde->fd] = ev.events;

        if (epoll_ctl(e->epoll_fd, epoll_ctl_type, fde->fd, &ev) < 0) {
            if (epoll_ctl_type != EPOLL_CTL_DEL || (errno != ENOENT && errno != EBADF)) {
                assert(0);
            }
        }

        switch (epoll_ctl_type) {
        case EPOLL_CTL_ADD:
            e->num_fds++;
            break;

        case EPOLL_CTL_DEL:
            e->num_fds--;
            break;

        default:
            break;
        }
    }
}

int epoll_event_driven_wait(epoll_t *e, int loopdelay) {
    struct epoll_event events[MAX_EVENTS];
    int num = epoll_wait(e->epoll_fd, events, MAX_EVENTS, loopdelay);
    if (num < 0) {
        if (errno_ignorable(errno)) {
            return 0;
        }
    }
    if (num == 0) {
        return -1;
    }
    int i;
    for (i = 0; i < num ; i ++) {
        fd_entry_t* fde = (fd_entry_t*)events[i].data.ptr;

        if (!fde->open) {
            continue;
        }

        cycle_call_handlers(e->cycle, fde, events[i].events & ~EPOLLOUT,
                events[i].events & ~EPOLLIN);
    }
    return 0;
}


int epoll_event_driven_wait_post(epoll_t *e, int loopdelay,
        struct list_head* accept_events, struct list_head* normal_events)
{
    struct epoll_event events[MAX_EVENTS];
    int num = epoll_wait(e->epoll_fd, events, MAX_EVENTS, loopdelay);

    if (num < 0) {
        if (errno_ignorable(errno)) {
            return 0;
        }
    }

    if (num == 0) {
        return -1;
    }

    int i;

    for (i = 0; i < num ; i ++) {
        fd_entry_t* fde = (fd_entry_t*)events[i].data.ptr;

        if (fde->timer.rbnode.key > 0) {
            rbtree_delete(e->cycle->rbtree, &fde->timer.rbnode);
        }

        fde->having_post_read_event = ((events[i].events & ~EPOLLOUT) != 0);
        fde->having_post_write_event = ((events[i].events & ~EPOLLIN) != 0);

        if (fde->listening) {

            list_add(&fde->post_event_node, accept_events);
        } else {
            list_add(&fde->post_event_node, normal_events);
        }
    }

    return 0;

}

void epoll_event_driven_close(epoll_t *e, fd_entry_t* fde) {
    epoll_set_event(e, fde, 0, 0);
}


void epoll_event_driven_shutdown(epoll_t *e) {
    close(e->epoll_fd);
}
