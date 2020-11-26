#ifndef __CYCLE_H
#define __CYCLE_H
#include "list.h"
#include "mem_buf.h"
#include "rbtree.h"
#include "epoll.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#define CYCLE_READ_EVENT (0x1)
#define CYCLE_WRITE_EVENT (0x2)

#define CYCLE_OK           (0)
#define CYCLE_ERROR       (-1)
#define CYCLE_NOMESSAGE   (-3)
#define CYCLE_TIMEOUT     (-4)
#define CYCLE_SHUTDOWN    (-5)
#define CYCLE_INPROGRESS  (-6)
#define CYCLE_ERR_CONNECT (-7)
#define CYCLE_ERR_DNS     (-8)
#define CYCLE_ERR_CLOSING (-9)
#define CYCLE_AGAIN_READ (-11)
#define CYCLE_AGAIN_WRITE (-12)
#define CYCLE_ZERO_RETURNED (-13)


#define CYCLE_NONBLOCKING        0x01
#define CYCLE_REUSEADDR          0x02
#define CYCLE_REUSEPORT          0x04
#ifndef SO_REUSEPORT
#define SO_REUSEPORT            15
#endif
struct cycle_s;
struct fd_entry_s;
struct timer_s;
typedef void EH(struct cycle_s *cycle, struct fd_entry_s *fde, void *data);
typedef void CCB(struct cycle_s *cycle, struct fd_entry_s *fde,
        int status, struct in_addr in_addr, int port, void* data);
typedef void WCB(struct cycle_s *cycle, struct fd_entry_s *fde, ssize_t size, 
                    int flag, void* data);
typedef void TOH(struct cycle_s *cycle, struct timer_s *timer, void *data);

struct timer_s {
    rbtree_node_t rbnode;
    TOH* timeout_handler;
    void* timeout_handler_data;
    struct list_head post_event_node;
};

#pragma pack(1)
struct fd_entry_s {
    int fd;
    EH* read_handler;
    void* read_handler_data;
    EH* write_handler;
    void* write_handler_data;
    struct timer_s timer;
    unsigned int open: 1;
    unsigned int listening: 1;
    unsigned int connecting: 1;
    unsigned int having_post_read_event: 1;
    unsigned int having_post_write_event: 1;
    struct list_head post_event_node;
    struct {
        mem_buf_t mb;
/*
        chain_t* out_chain_curr;
        chain_t* out_chain_tail;
        int chain_need_free;
*/
        size_t offset;
        WCB* callback;
        void* callback_data;
        time_t timeout_length;
        TOH* timeout_handler;
        void* timeout_handler_data;
    } write_state;
    struct in_addr local_addr;
    short local_port;
    int flags;
}; 
#pragma pack()
typedef struct fd_entry_s fd_entry_t;

struct cycle_s {
    epoll_t *e;
    rbtree_t *rbtree;
    fd_entry_t *fde_table;
};

typedef struct cycle_s cycle_t;

struct cycle_connect_state_s {
    fd_entry_t* fde;
    struct in_addr in_addr;
    short port;
    time_t connect_timeout;
    CCB* callback;
    void *cbdata;
};

typedef struct cycle_connect_state_s cycle_connect_state_t;

cycle_t *init_cycle();
void cycle_set_timeout(cycle_t *cycle, struct timer_s *timer, int timeout_msec,
        TOH* timeout_handler, void* timeout_handler_data);
void cycle_check_timeouts(cycle_t* cycle);
void process_posted_events(cycle_t *cycle, struct list_head *events);
void cycle_call_handlers(cycle_t *cycle, fd_entry_t *fde, int is_read, int is_write);
void cycle_set_event(cycle_t *cycle, fd_entry_t *fde, int event_rw, EH *handler,
        void *handler_data);
void cycle_wait_post_event(cycle_t *cycle, struct list_head *accept_events,
        struct list_head *normal_events);
int cycle_wait(cycle_t* cycle);
void close_finish(cycle_t* cycle, fd_entry_t* fde);
void cycle_close(cycle_t* cycle, fd_entry_t* fde);
int set_nonblocking(int fd);
void set_reuseaddr(int fd);
void set_reuseport(int fd);
void set_close_on_exec(int fd);
int errno_ignorable(int ierrno);
int open_listen_fd(int flags, struct in_addr myaddr,
        unsigned short myport, int close_on_exec);
fd_entry_t *cycle_create_listen_fde(cycle_t *cycle, int new_socket);
int cycle_get_socket_port_from_fd(int fd);
void fde_def_init(fd_entry_t* fde, int flags, int new_socket, cycle_t *cycle);
fd_entry_t *cycle_open_tcp(cycle_t* cycle, int flags, struct in_addr myaddr,
        unsigned short myport, int rcvbuf_size, int sndbuf_size);
int cycle_bind_tcp(fd_entry_t* fde, long ip);
void set_linger(int fd, int linger);
fd_entry_t* cycle_open_tcp_nobind(cycle_t* cycle, int flags, unsigned short myport,
        int rcvbuf_size, int sndbuf_size);
int cycle_listen(cycle_t* cycle, fd_entry_t* fde);
fd_entry_t *cycle_accept(cycle_t *cycle, fd_entry_t *listen_fde,
        struct sockaddr_in *pn, struct sockaddr_in *me);
void cycle_write(cycle_t* cycle, fd_entry_t* fde, char* buf, size_t size, size_t capacity,
                WCB* callback, void* cbdata);
int cycle_read(fd_entry_t* fde, char* buf, size_t size);
void cycle_connect_callback(cycle_t* cycle, cycle_connect_state_t* cs, int status);
void cycle_connect(cycle_t* cycle, fd_entry_t* fde, struct in_addr ip,
        unsigned short port, time_t connect_timeout, CCB* callback, void* data);
fd_entry_t* cycle_find_fde(cycle_t* cycle, int fd);


#endif
