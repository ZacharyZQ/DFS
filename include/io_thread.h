#ifndef __IO_THREAD_H
#define __IO_THREAD_H
#define IO_READ ((int)1)
#define IO_WRITE ((int)2)
typedef struct {
    int type;
    int fd;
    block_t *bt;
    int offset;
    int length;
    char *buf;
    void *callback;
    void *data;
    cycle_t *main_cycle;
    cycle_t *io_cycle;
    int io_id;
    int block_id;
    int status;
} io_op_t;

typedef struct {
    pthread_t thread;
    pthread_attr_t thread_attr;
    char *thread_stack;
    int io_pipe_read;
    int io_pipe_write;
    fd_entry_t *io_pipe_read_fde;
    fd_entry_t *io_pipe_write_fde;
    cycle_t *cycle;
} io_thread_t;


void io_thread_init(cycle_t *cycle);
void main_distribute_block(cycle_t *cycle, int io_id, int block_id, int fd,
        int offset, int length, block_t *bt, void *callback, void *data);

#endif
