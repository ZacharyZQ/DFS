#include "dfs_head.h"
#define CURRENT_MSEC \
    (unsigned long)(current_time_tv.tv_sec * 1000 + \
            current_time_tv.tv_usec / 1000)
static void cycle_connect_timeout(cycle_t* cycle, struct timer_s* timer, void* data);

cycle_t *init_cycle() {
    cycle_t *cycle = calloc(1, sizeof(cycle_t));
    cycle->e = epoll_event_driven_init();
    cycle->e->cycle = cycle;
    cycle->rbtree = calloc(1, sizeof(rbtree_t));
    rbtree_init(cycle->rbtree, rbtree_insert_timer_value);
    cycle->fde_table = (fd_entry_t *)calloc(65536, sizeof(fd_entry_t));
    return cycle;
}

void cycle_set_timeout(cycle_t *cycle, struct timer_s *timer, int timeout_msec,
        TOH* timeout_handler, void* timeout_handler_data)
{
    if (timer->rbnode.key > 0) {
        timer->timeout_handler = NULL;
        timer->timeout_handler_data = NULL;
        rbtree_delete(cycle->rbtree, &timer->rbnode);
    }
    if (timeout_msec > 0) {
        timer->timeout_handler = timeout_handler;
        timer->timeout_handler_data = timeout_handler_data;
        timer->rbnode.key = CURRENT_MSEC + timeout_msec;
        rbtree_insert(cycle->rbtree, &timer->rbnode);
    }
}

void cycle_check_timeouts(cycle_t* cycle) {
    while (1) {
        if (rbtree_empty(cycle->rbtree)) {
            break;
        }
        rbtree_node_t* node = rbtree_min(cycle->rbtree->root,
                cycle->rbtree->sentinel);
        if (node->key > CURRENT_MSEC) {
            break;
        }
        struct timer_s *timer = (struct timer_s *)node;
        assert(timer->timeout_handler);
        rbtree_delete(cycle->rbtree, node);
        timer->timeout_handler(cycle, timer, timer->timeout_handler_data);
    }
}

void process_posted_events(cycle_t *cycle, struct list_head *events) {
    struct list_head *p;
    list_for_each_clear(p, events) {
        fd_entry_t *fde = list_entry(p, fd_entry_t, post_event_node);
        assert(fde->open);
        cycle_call_handlers(cycle, fde, fde->having_post_read_event,
                fde->having_post_write_event);
    }
}

void cycle_call_handlers(cycle_t *cycle, fd_entry_t *fde, int is_read, int is_write) {
    if (!is_read && !is_write) {
        assert(0);
    }
    if (fde->timer.rbnode.key != 0) {
        rbtree_delete(cycle->rbtree, &fde->timer.rbnode);
    }

    if (fde->read_handler && is_read) {
        EH* hdl = fde->read_handler;
        void* hdl_data = fde->read_handler_data;
        fde->read_handler = NULL;
        fde->read_handler_data = NULL;
        hdl(cycle, fde, hdl_data);
    }

    if (!fde->open) {
        return;
    }

    if (fde->write_handler && is_write) {
        EH* hdl = fde->write_handler;
        void* hdl_data = fde->write_handler_data;
        fde->write_handler = NULL;
        fde->write_handler_data = NULL;
        hdl(cycle, fde, hdl_data);
    }

    if (!fde->open) {
        return;
    }

    epoll_set_event(cycle->e, fde, fde->read_handler != NULL, fde->write_handler != NULL);
}

void cycle_set_event(cycle_t *cycle, fd_entry_t *fde, int event_rw, EH *handler,
        void *handler_data)
{
    if (handler) {
        assert(fde->timer.rbnode.key > 0);
    }
    assert(fde->open);
    if (event_rw & CYCLE_READ_EVENT) {
        fde->read_handler = handler;
        fde->read_handler_data = handler_data;
    }
    if (event_rw & CYCLE_WRITE_EVENT) {
        fde->write_handler = handler;
        fde->write_handler_data = handler_data;
    }
    int is_read = (fde->read_handler != NULL);
    int is_write = (fde->write_handler != NULL);
    assert(fde->fd >= 0);
    epoll_set_event(cycle->e, fde, is_read, is_write);
}

void cycle_wait_post_event(cycle_t *cycle, struct list_head *accept_events,
        struct list_head *normal_events)
{
    time_t loopdelay;

    if (rbtree_empty(cycle->rbtree)) {
        loopdelay = 125;
    } else {
        rbtree_node_t *node = rbtree_min(cycle->rbtree->root,
                cycle->rbtree->sentinel);
        assert(node != cycle->rbtree->sentinel);
        loopdelay = node->key - CURRENT_MSEC;
        
        if (loopdelay < 0 || loopdelay > 125) {
            loopdelay = 125; 
        }   
    }             
    
    epoll_event_driven_wait_post(cycle->e, loopdelay,
            accept_events, normal_events);
}

int cycle_wait(cycle_t* cycle) {
    time_t loopdelay;

    if (rbtree_empty(cycle->rbtree)) {
        loopdelay = 1000;
    } else {
        rbtree_node_t *node = rbtree_min(cycle->rbtree->root,
                cycle->rbtree->sentinel);
        assert(node != cycle->rbtree->sentinel);
        loopdelay = node->key > CURRENT_MSEC ? node->key - CURRENT_MSEC : 0;
    }
    cycle_check_timeouts(cycle);
    int rc = epoll_event_driven_wait(cycle->e, loopdelay);

    return rc;
}

void close_finish(cycle_t* cycle, fd_entry_t* fde) {
    epoll_event_driven_close(cycle->e, fde);
    assert(cycle->e->old_fd_events[fde->fd] == 0);
    if (fde->timer.rbnode.key != 0) {
        assert(fde->timer.rbnode.key > 0);
        rbtree_delete(cycle->rbtree, &fde->timer.rbnode);
        assert(fde->timer.rbnode.left == NULL);
        assert(fde->timer.rbnode.right == NULL);
        assert(fde->timer.rbnode.parent == NULL);
    }
    assert(fde->open == 1);
    list_del(&fde->post_event_node);
    list_del(&fde->timer.post_event_node);
    int fd = fde->fd;
    memset(fde, 0, sizeof(fd_entry_t));
    INIT_LIST_HEAD(&fde->post_event_node);
    INIT_LIST_HEAD(&fde->post_event_node);
    INIT_LIST_HEAD(&fde->timer.post_event_node);
    close(fd);
}

void cycle_close(cycle_t* cycle, fd_entry_t* fde) {
    close_finish(cycle, fde);
}

int set_nonblocking(int fd) {
    int flags = 0;
    int dummy = 0;
    if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    return 0;
}

void set_reuseaddr(int fd) {
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));
}

void set_reuseport(int fd) {
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const void *) &on, sizeof(on));
}

void set_close_on_exec(int fd) {
    int flags = 0, dummy = 0;  
    if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) { 
        return;
    }    
    fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

int errno_ignorable(int ierrno) {
    switch (ierrno) {
    case EINPROGRESS:
    case EWOULDBLOCK:
#if EAGAIN != EWOULDBLOCK
    case EAGAIN:
#endif    
    case EALREADY:
    case EINTR:
#ifdef ERESTART
    case ERESTART:
#endif  
        return 1;

    default:
        return 0;
    }
}

int open_listen_fd(int flags, struct in_addr myaddr,
        unsigned short myport, int close_on_exec)
{
    int new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (flags & CYCLE_NONBLOCKING) {
        if (set_nonblocking(new_socket) != 0) {
            close(new_socket);
            return -1;
        }
    }
    if (flags & CYCLE_REUSEADDR) {
        set_reuseaddr(new_socket);
    }
    if (flags & CYCLE_REUSEPORT) {
        set_reuseport(new_socket);
    }
    if (close_on_exec) {
        set_close_on_exec(new_socket);
    }
    struct sockaddr_in S;
    S.sin_family = AF_INET;
    S.sin_port = htons(myport);
    S.sin_addr = myaddr;

    if (bind(new_socket, (struct sockaddr*)&S, sizeof(S)) < 0) {
        close(new_socket);
        return -1;
    }
    return new_socket;
}

fd_entry_t *cycle_create_listen_fde(cycle_t *cycle, int new_socket) {
    fd_entry_t* fde = &cycle->fde_table[new_socket];
    INIT_LIST_HEAD(&fde->post_event_node);
    INIT_LIST_HEAD(&fde->timer.post_event_node);
    fde->listening = 1;
    fde->fd = new_socket;
    fde->open = 1;
    assert(cycle->e->old_fd_events[new_socket] == 0);
    return fde;
}


int cycle_get_socket_port_from_fd(int fd) {
    int port = 0;
    struct sockaddr_in sin;
    socklen_t len;

    len = sizeof(sin);
    memset(&sin, '\0', len);
    if (getsockname(fd, (struct sockaddr*) &sin, &len) == 0) {
        if (sin.sin_family == AF_INET) {
            port = ntohs(sin.sin_port);
        }
    }
    return port;
}

void fde_def_init(fd_entry_t* fde, int flags, int new_socket, cycle_t *cycle) {
    fde->flags = flags;
    if (flags & CYCLE_NONBLOCKING) {
        if (set_nonblocking(new_socket) != 0) {
            close(new_socket);
            assert(0);
        }
    }

    if (flags & CYCLE_REUSEADDR) {
        set_reuseaddr(new_socket);
    }

    INIT_LIST_HEAD(&fde->post_event_node);
    INIT_LIST_HEAD(&fde->timer.post_event_node);
    fde->listening = 0;
    fde->fd = new_socket;
    fde->open = 1;
}

fd_entry_t *cycle_open_tcp(cycle_t* cycle, int flags, struct in_addr myaddr,
        unsigned short myport, int rcvbuf_size, int sndbuf_size)
{
    int new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket < 0) {
        return NULL;
    }
    set_close_on_exec(new_socket);
    if (flags & CYCLE_NONBLOCKING) {
        if (set_nonblocking(new_socket) != 0) {
            close(new_socket);
            abort();
            return NULL;
        }
    }

    if (flags & CYCLE_REUSEADDR) {
        set_reuseaddr(new_socket);
    }

    struct sockaddr_in S;
    S.sin_family = AF_INET;
    S.sin_port = htons(myport);
    S.sin_addr = myaddr;

    if (bind(new_socket, (struct sockaddr*)&S, sizeof(S)) < 0) {
        close(new_socket);
        return NULL;
    }

    if (rcvbuf_size > 0) {
        setsockopt(new_socket, SOL_SOCKET, SO_RCVBUF, (char*)&rcvbuf_size,
                sizeof(rcvbuf_size));
    }

    if (sndbuf_size > 0) {
        setsockopt(new_socket, SOL_SOCKET, SO_SNDBUF, (char*)&sndbuf_size,
                sizeof(sndbuf_size));
    }

    fd_entry_t* fde = &cycle->fde_table[new_socket];

    fde->local_addr = myaddr;
    fde->local_port = myport;
/*
    fde->rcvbuf_size = rcvbuf_size;
    fde->sndbuf_size = sndbuf_size;
*/

    if (fde->open) {
        assert(0);
    }
    fde_def_init(fde, flags, new_socket, cycle);

    if (cycle->e->old_fd_events[fde->fd] != 0) {
        assert(0);
    }
    return fde;
}

int cycle_bind_tcp(fd_entry_t* fde, long ip)
{
    struct in_addr myaddr;
    myaddr.s_addr = ip;
    struct sockaddr_in S;                                       
    S.sin_family = AF_INET;                                     
    S.sin_port = htons(fde->local_port);
    S.sin_addr = myaddr;                                        

    int socket_fd = fde->fd;
    if (bind(socket_fd, (struct sockaddr*)&S, sizeof(S)) < 0) {
        return -1;
    }                                                           
    //fde->local_addr = myaddr;
    return 0;
}

typedef struct {
    int l_onoff;
    int l_linger;
} socket_linger_t;

void set_linger(int fd, int linger) {
    if (linger < 0) {
        return;
    }

    socket_linger_t so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = linger;

    setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
}

fd_entry_t* cycle_open_tcp_nobind(cycle_t* cycle, int flags, unsigned short myport,
        int rcvbuf_size, int sndbuf_size)
{
    int new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket < 0) {
        return NULL;
    }
    set_close_on_exec(new_socket);

    if (flags & CYCLE_NONBLOCKING) {
        if (set_nonblocking(new_socket) != 0) {
            close(new_socket);
            abort();
            return NULL;
        }
    }

    if (flags & CYCLE_REUSEADDR) {
        set_reuseaddr(new_socket);
    }

    if (rcvbuf_size > 0) {
        setsockopt(new_socket, SOL_SOCKET, SO_RCVBUF,
                (char*)&rcvbuf_size, sizeof(rcvbuf_size));
    }

    if (sndbuf_size > 0) {
        setsockopt(new_socket, SOL_SOCKET, SO_SNDBUF,
                (char*)&sndbuf_size, sizeof(sndbuf_size));
    }

    fd_entry_t* fde = &cycle->fde_table[new_socket];
    fde->local_port = myport;
    //fde->rcvbuf_size = rcvbuf_size;
    //fde->sndbuf_size = sndbuf_size;
    if (fde->open) {
        assert(0);
    }
    fde_def_init(fde, flags, new_socket, cycle);
    if (cycle->e->old_fd_events[fde->fd] != 0) {
        assert(0);
    }
    return fde;
}

int cycle_listen(cycle_t* cycle, fd_entry_t* fde) {
    assert(fde->fd >= 0);
    int x = 0;
    if ((x = listen(fde->fd, 65536 >> 2)) < 0) {
        assert(0);
    }
    fde->listening = 1;
    return x;
}

fd_entry_t *cycle_accept(cycle_t *cycle, fd_entry_t *listen_fde,
        struct sockaddr_in *pn, struct sockaddr_in *me)
{
    int sock = 0;
    struct sockaddr_in P;
    struct sockaddr_in M;
    socklen_t Slen;

    Slen = sizeof(P);

    if ((sock = accept(listen_fde->fd, (struct sockaddr*) &P, &Slen)) < 0) {
       return NULL;
    }

    set_close_on_exec(sock);
    if (set_nonblocking(sock) != 0) {
        close(sock);
        abort();
        return NULL;
    }

    if (pn) {
        *pn = P;
    }

    Slen = sizeof(M);
    memset(&M, '\0', Slen);
    getsockname(sock, (struct sockaddr*) &M, &Slen);

    if (me) {
        *me = M;
    }

    fd_entry_t* fde = &cycle->fde_table[sock];
    if (fde->open) {
        assert(0);
    }
    fde_def_init(fde, CYCLE_NONBLOCKING, sock, cycle);
    assert(cycle->e->old_fd_events[fde->fd] == 0);
    return fde;
}

static void cycle_write_callback(cycle_t* cycle, fd_entry_t* fde, int state) {
    WCB* callback = fde->write_state.callback;
    void* cbdata = fde->write_state.callback_data;
    size_t offset = fde->write_state.offset;

    if(fde->write_state.mb.buf) {
        mem_buf_clean(&fde->write_state.mb);
    }
 
    fde->write_state.callback = NULL;
    fde->write_state.callback_data = NULL;
    fde->write_state.offset = 0;
/*
    fde->write_state.out_chain_curr = NULL;
    fde->write_state.out_chain_tail = NULL;
*/

    callback(cycle, fde, offset, state, cbdata);
}

static void cycle_handle_write(cycle_t* cycle, fd_entry_t* fde, void* datanotused) {
    assert(!fde->write_handler);
    assert((ssize_t)fde->write_state.mb.size > fde->write_state.offset);
    size_t nleft = fde->write_state.mb.size - fde->write_state.offset;
    assert(nleft > 0);
    ssize_t len = write(fde->fd, fde->write_state.mb.buf + fde->write_state.offset, nleft);

    if (len <= 0) {
        if (errno_ignorable(errno)) {
            cycle_set_timeout(cycle, &fde->timer, fde->write_state.timeout_length,
                             fde->write_state.timeout_handler, fde->write_state.timeout_handler_data);
            cycle_set_event(cycle, fde, CYCLE_WRITE_EVENT, cycle_handle_write, NULL);
        } else {
            //peer closed
            cycle_write_callback(cycle, fde, CYCLE_ERROR);
        }
    } else {
        fde->write_state.offset += len;
        assert(fde->write_state.offset <= (ssize_t)fde->write_state.mb.size);

        if (fde->write_state.offset == (ssize_t)fde->write_state.mb.size) {
            cycle_write_callback(cycle, fde, CYCLE_OK);
        } else {
            cycle_set_timeout(cycle, &fde->timer, fde->write_state.timeout_length,
                             fde->write_state.timeout_handler, fde->write_state.timeout_handler_data);
            cycle_set_event(cycle, fde, CYCLE_WRITE_EVENT, cycle_handle_write, NULL);
        }
    }
}

#if 0
static chain_t* cycle_dump_chain_into_iovec(iovec_t* vec, chain_t* in)
{
    size_t total = 0;
    // currently how many bytes this buf has
    size_t size = 0;
    // currently how many bufs have been dumped (compard to nalloc)
    unsigned int num = 0;
    struct iovec* curr_iov = NULL;
    char* prev_last = NULL;

    for(/* void */; in; in = in->next)
    {
        // check file first
        if(in->buf.in_file)
        {
            error_log(LOG_DEBUG, 
                        "cycle_dump_chain_into_iovec: file buf found, "
                        "start=%ld end=%ld path=%p, break\n", 
                        in->buf.file_start, in->buf.file_end, in->buf.path);
            break;
        }

        size = in->buf.last - in->buf.pos;
                
        if(in->buf.pos == NULL || size == 0)
        {
            error_log(LOG_DEBUG, 
                        "cycle_dump_chain_into_iovec: buf.pos=%p size = 0, skipped\n", in->buf.pos);
            continue;
        }

        if(prev_last == in->buf.pos)
        {
            error_log(LOG_DEBUG, 
                    "cycle_dump_chain_into_iovec: prev_last == in->buf.pos %p\n", prev_last);
            curr_iov->iov_len += size;
        }
        else
        {
            if(num == vec->nalloc)
            {
                error_log(LOG_DEBUG, 
                    "cycle_dump_chain_into_iovec: num==vec->nalloc break\n");

                break;
            }

            curr_iov = &vec->iovs[num++];
            curr_iov->iov_base = (void*)in->buf.pos;
            curr_iov->iov_len = size;
        } 
        
        prev_last = in->buf.pos + size;
        total += size;
    }
    
    vec->count = num;
    vec->size = total;
    
    return in;
}

#define IOV_ARRAY_ALLOC_NUM 64
static void cycle_handle_writev_chain(cycle_t* cycle, fd_entry_t* fde, void* datanotused)
{
    assert(!fde->write_handler);
    assert(fde->write_state.out_chain_curr);
    
    // used to check if writev is not ready or iovs is full
    size_t send = 0;
    size_t prev_send = 0;
    // how many bytes actuall sent
    ssize_t sent = 0;
    iovec_t vec;
    // this array may be used for more than once
    struct iovec iovs[IOV_ARRAY_ALLOC_NUM];
    
    vec.iovs = iovs;
    vec.nalloc = IOV_ARRAY_ALLOC_NUM;
    
    while (1) {
        prev_send = send;
        // dump bufs into iovs, in_file buf will not be dumped, and dump will stop
        cycle_dump_chain_into_iovec(&vec, fde->write_state.out_chain_curr);
        send += vec.size;
        sent = writev(fde->fd, vec.iovs, vec.count);
        
        if(sent < 0) {
            if (errno_ignorable(errno)) {
                cycle_set_timeout(cycle, &fde->timer, fde->write_state.timeout_length,
                        fde->write_state.timeout_handler, fde->write_state.timeout_handler_data);
                cycle_set_event(cycle, fde, CYCLE_WRITE_EVENT, cycle_handle_writev_chain, NULL);
            } else {
                cycle_write_callback(cycle, fde, CYCLE_ERROR);
            }
            return;
        }
        fde->write_state.offset += sent;
        // update out_chain_curr
        fde->write_state.out_chain_curr = chain_update_sent(cycle, 
                fde->write_state.out_chain_curr,                                                                sent, fde->write_state.chain_need_free);
        // if writev is not ready then set write event again
        if (send - prev_send != sent) {
            cycle_set_timeout(cycle, &fde->timer, fde->write_state.timeout_length,
                    fde->write_state.timeout_handler, fde->write_state.timeout_handler_data);
            cycle_set_event(cycle, fde, CYCLE_WRITE_EVENT, cycle_handle_writev_chain, NULL);
            return;
        }
        // if all bufs have been sent, callback with CYCLE_OK
        if(fde->write_state.out_chain_curr == NULL) {
            cycle_write_callback(cycle, fde, CYCLE_OK);
            return;
        }
        // gets here if out_chain_curr still has some bufs waiting to be sent, loop again
    }
}
#endif

#define MEM_BUF_MAX_SIZE    (2*1000*1024*1024)
void cycle_write(cycle_t* cycle, fd_entry_t* fde, char* buf, size_t size, size_t capacity,
                WCB* callback, void* cbdata) 
{

    assert(size > 0);
    assert(fde->timer.rbnode.key > 0);
    if (fde->write_state.mb.size > 0) {
        mem_buf_append(&fde->write_state.mb, buf, size);
        free(buf);
        return;
    }
    fde->write_state.mb.buf = buf;
    fde->write_state.mb.size = size;
    fde->write_state.mb.capacity = capacity;
    fde->write_state.mb.max_capacity = MEM_BUF_MAX_SIZE;

    fde->write_state.offset = 0;
    fde->write_state.callback = callback;
    fde->write_state.callback_data = cbdata;
    fde->write_state.timeout_length = fde->timer.rbnode.key > CURRENT_MSEC 
            ? fde->timer.rbnode.key - CURRENT_MSEC :60000;
    fde->write_state.timeout_handler = fde->timer.timeout_handler;
    fde->write_state.timeout_handler_data = fde->timer.timeout_handler_data;

    cycle_set_event(cycle, fde, CYCLE_WRITE_EVENT, cycle_handle_write, NULL);
}
#if 0
void cycle_writev_chain(cycle_t* cycle, fd_entry_t* fde, chain_t* in, 
                    WCB* callback, void* cbdata, int chain_need_free)
{
    assert(fde->timer.key > 0);
    if(fde->write_state.out_chain_tail == NULL) {
        fde->write_state.out_chain_tail = in;
    }

    if(fde->write_state.out_chain_curr != NULL && chain_need_free) {
        drd_chain_insert_after(fde->write_state.out_chain_tail, in);
        fde->write_state.out_chain_tail = in;
        return;
    }

    fde->write_state.out_chain_curr = in;
    fde->write_state.chain_need_free = chain_need_free;

    fde->write_state.offset = 0;
    fde->write_state.callback = callback;
    fde->write_state.callback_data = cbdata;
    fde->write_state.timeout_length = fde->timer.key > CURRENT_MSEC
            ? fde->timer.key - CURRENT_MSEC : 60000;
    fde->write_state.timeout_handler = fde->timer.timeout_handler;
    fde->write_state.timeout_handler_data = fde->timer.timeout_handler_data;
    cycle_set_event(cycle, fde, CYCLE_WRITE_EVENT, cycle_handle_writev_chain, NULL);
}
#endif

int cycle_read(fd_entry_t* fde, char* buf, size_t size) {
    ssize_t ret = read(fde->fd, buf, size);

    if (ret > 0) {
        return ret;
    }
    //peer closed
    else if (ret == 0) {
        return CYCLE_ZERO_RETURNED;
    } else if (ret < 0 && errno_ignorable(errno)) {
        return CYCLE_AGAIN_READ;
    } else {
        return CYCLE_ERROR;
    }
}

void cycle_connect_callback(cycle_t* cycle, cycle_connect_state_t* cs, int status) 
{
    CCB* callback = cs->callback;
    void* data = cs->cbdata;
    fd_entry_t* fde = cs->fde;

    struct sockaddr_in local;
    socklen_t len = sizeof(struct sockaddr_in);
    if(status == CYCLE_OK && (getsockname(fde->fd, (struct sockaddr*)&local, &len) == 0))
    {
        fde->local_addr = local.sin_addr;
        fde->local_port = ntohs(local.sin_port);
    }

    cs->callback = NULL;
    cs->cbdata = NULL;

    struct in_addr in_addr = cs->in_addr;
    int port               = cs->port;

    free(cs);

#if 0
    if (drd_cbdata_valid(data)) {
        callback(cycle, status == CYCLE_OK ? fde : NULL,
                status, in_addr, port, data);
    } else if (status == CYCLE_OK) {
        cycle_close(cycle, fde);
    }
#endif
    if (status != CYCLE_OK) {
        cycle_close(cycle, fde);
    }
    callback(cycle, status == CYCLE_OK ? fde : NULL,
            status, in_addr, port, data);

    //drd_cbdata_unlock(data);
}

static int cycle_connect_addr(fd_entry_t* fde, const struct sockaddr_in* address) {
    //char ip[20];
    int status = CYCLE_OK;
    int x = 0;
    int err = 0;
    socklen_t errlen;
    assert(ntohs(address->sin_port) != 0);
    if (fde->connecting == 0) {
        fde->connecting = 1;
        x = connect(fde->fd, (struct sockaddr*) address, sizeof(*address));
        if (x < 0) {
            if (errno_ignorable(errno)) {
                return CYCLE_INPROGRESS;
            }
        }
    } else {
        errlen = sizeof(err);
        if (getsockopt(fde->fd, SOL_SOCKET, SO_ERROR, &err, &errlen) != 0) {
            return CYCLE_ERROR;
        }
    }

    if (err == 0 || err == EISCONN) {
        status = CYCLE_OK;
    } else if (errno_ignorable(err)) {
        status = CYCLE_INPROGRESS;
    } else {
        status = CYCLE_ERROR;
    }
    return status;
}

static void cycle_connect_handle(cycle_t* cycle, fd_entry_t* fde, void* data) {
    cycle_connect_state_t* cs = (cycle_connect_state_t*)data;
    assert(cs->fde == fde);
    struct sockaddr_in sai;
    char                        ip[20];
    sai.sin_family = AF_INET;
    sai.sin_addr = cs->in_addr;
    sai.sin_port = htons(cs->port);

    inet_ntop(AF_INET, &cs->in_addr, ip, 20);

    switch (cycle_connect_addr(cs->fde, &sai)) {
    case CYCLE_INPROGRESS:
        cycle_set_timeout(cycle, &cs->fde->timer,
                cs->connect_timeout * 1000, cycle_connect_timeout, cs);
        cycle_set_event(cycle, cs->fde, CYCLE_WRITE_EVENT, cycle_connect_handle, cs);
        break;

    case CYCLE_OK:
        cycle_connect_callback(cycle, cs, CYCLE_OK);
        break;

    default:
        cycle_close(cycle, cs->fde);
        cycle_connect_callback(cycle, cs, CYCLE_ERR_CONNECT);
        break;
    }
}

static void cycle_connect_timeout(cycle_t* cycle, struct timer_s* timer, void* data) {
    cycle_connect_state_t* cs = (cycle_connect_state_t*)data;
    log(LOG_RUN_ERROR, "fd %d\n", cs->fde->fd);
    cycle_close(cycle, cs->fde);
    cycle_connect_callback(cycle, cs, CYCLE_ERR_CONNECT);
}

void cycle_connect(cycle_t* cycle, fd_entry_t* fde, struct in_addr ip, 
        unsigned short port, time_t connect_timeout, CCB* callback, void* data)
{
    assert(fde->connecting == 0 || fde->connecting == 1);
    cycle_connect_state_t* cs = calloc(1, sizeof(cycle_connect_state_t));
    cs->fde = fde;
    cs->in_addr = ip;
    cs->port = port;
    cs->connect_timeout = connect_timeout;
    cs->callback = callback;
    cs->cbdata = data;
    cycle_connect_handle(cycle, fde, cs);

}

fd_entry_t* cycle_find_fde(cycle_t* cycle, int fd) {
    if (fd < 0) {
        return NULL;
    }
    return &cycle->fde_table[fd];
}

int safe_inet_addr(const char* buf, struct in_addr* addr) {
    char addrbuf[32];
    int a1 = 0, a2 = 0, a3 = 0, a4 = 0; 
    struct in_addr A;
    char x;

    if (sscanf(buf, "%d.%d.%d.%d%c", &a1, &a2, &a3, &a4, &x) != 4) { 
        return 0;
    }    

    if (a1 < 0 || a1 > 255) {
        return 0;
    }    

    if (a2 < 0 || a2 > 255) {
        return 0;
    }    

    if (a3 < 0 || a3 > 255) {
        return 0;
    }    

    if (a4 < 0 || a4 > 255) {
        return 0;
    }

    snprintf(addrbuf, 32, "%d.%d.%d.%d", a1, a2, a3, a4);
    A.s_addr = inet_addr(addrbuf);

    if (addr) {
        addr->s_addr = A.s_addr;
    }

    return 1;
}
