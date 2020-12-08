#include "dfs_head.h"
void null_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data);
void null_cycle_write_callback(cycle_t* cycle, fd_entry_t* fde, ssize_t size,
                                       int flag, void* data);
void io_process_pipe_request(cycle_t *cycle, fd_entry_t *fde, void* data);
io_thread_t *io_thread_start(int io_pipe_read, int io_pipe_write);
void master_process_pipe_callback(cycle_t *cycle, fd_entry_t *fde, void* data);
void main_distribute_block_done(cycle_t *cycle, io_op_t *op);
void io_process_distribute_block(cycle_t *cycle, io_op_t *op);
void io_process_distribute_block_done(io_op_t *op, int slave_id, int status);
void main_get_block_done(cycle_t *cycle, io_op_t *op);
void io_process_get_block(cycle_t *cycle, io_op_t *op);
void io_process_get_block_done(io_op_t *op, int slave_id, int status);
void block_mark_slave_available(block_t *bt, int slave_id); 
void block_mark_slave_unavailable(block_t *bt, int slave_id);

void op_destory(io_op_t *op) {
    if (op->buf) {
        free(op->buf);
    }
    free(op);
}

void* io_process_routine(void* param) {
    io_thread_t *iot = (io_thread_t *)param;
    iot->cycle = init_cycle();
    iot->io_pipe_read_fde = &iot->cycle->fde_table[iot->io_pipe_read];
    iot->io_pipe_write_fde = &iot->cycle->fde_table[iot->io_pipe_write];
    fde_def_init(iot->io_pipe_read_fde, CYCLE_NONBLOCKING, iot->io_pipe_read, iot->cycle);
    fde_def_init(iot->io_pipe_write_fde, CYCLE_NONBLOCKING, iot->io_pipe_write, iot->cycle);
    cycle_set_timeout(iot->cycle, &iot->io_pipe_read_fde->timer, 1000, null_timeout, NULL);
    cycle_set_event(iot->cycle, iot->io_pipe_read_fde, CYCLE_READ_EVENT,
            io_process_pipe_request, NULL);
    
    while (1) {
        LIST_HEAD(accept_events);
        LIST_HEAD(normal_events);
        cycle_wait_post_event(iot->cycle, &accept_events, &normal_events);
        process_posted_events(iot->cycle, &accept_events);
        process_posted_events(iot->cycle, &normal_events);
        cycle_check_timeouts(iot->cycle);
    }
}

void io_thread_init(cycle_t *cycle) {
    int i;
    for (i = 0; i < IO_THREAD_NUM; i ++) {
        int m2i[2];
        int i2m[2];
        pipe(m2i);
        pipe(i2m);
        set_nonblocking(m2i[0]);
        set_nonblocking(m2i[1]);
        set_nonblocking(i2m[0]);
        set_nonblocking(i2m[1]);
        set_close_on_exec(m2i[0]);
        set_close_on_exec(m2i[1]);
        set_close_on_exec(i2m[0]);
        set_close_on_exec(i2m[1]);
        log(LOG_DEBUG, "pipe m2i %d %d, i2m %d %d\n", m2i[0], m2i[1], i2m[0], i2m[1]);
        master.iot_group[i] = io_thread_start(m2i[0], i2m[1]);
        master.m_read_fde[i] = &cycle->fde_table[i2m[0]];
        master.m_write_fde[i] = &cycle->fde_table[m2i[1]];
        fde_def_init(master.m_read_fde[i], CYCLE_NONBLOCKING, i2m[0], cycle);
        fde_def_init(master.m_write_fde[i], CYCLE_NONBLOCKING, m2i[1], cycle);
        cycle_set_timeout(cycle, &master.m_read_fde[i]->timer, 1000, null_timeout, NULL);
        cycle_set_event(cycle, master.m_read_fde[i], CYCLE_READ_EVENT, master_process_pipe_callback, NULL);
    }
}

io_thread_t *io_thread_start(int io_pipe_read, int io_pipe_write) {
    io_thread_t *iot = calloc(1, sizeof(io_thread_t));
    iot->io_pipe_read = io_pipe_read;
    iot->io_pipe_write = io_pipe_write;
    pthread_attr_init(&iot->thread_attr);
    pthread_attr_setscope(&iot->thread_attr, PTHREAD_SCOPE_SYSTEM);
    iot->thread_stack = (char*)calloc(1048576 * 2, sizeof(char));
    pthread_attr_setstack(&iot->thread_attr, iot->thread_stack, 1048576 * 2);
    pthread_create(&iot->thread, &iot->thread_attr, io_process_routine, iot);
    pthread_detach(iot->thread);
    return iot;
}


void null_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
}
void null_cycle_write_callback(cycle_t* cycle, fd_entry_t* fde, ssize_t size,
                                       int flag, void* data)
{
}

void master_process_pipe_callback(cycle_t *cycle, fd_entry_t *fde, void* data) {
    char buf[sizeof(io_op_t *)];
    ssize_t len;
    while ((len = read(fde->fd, buf, sizeof(io_op_t *))) > 0) { 
        io_op_t** addr_of_op = (io_op_t**) buf; 
        io_op_t* op = *addr_of_op;

        if (!op) {
            continue;
        }
        switch(op->type) {
        case IO_WRITE:
            main_distribute_block_done(cycle, op);
            break;
        case IO_READ:
            main_get_block_done(cycle, op);
            break;
        }
        op_destory(op);
    }
    cycle_set_timeout(cycle, &fde->timer, 1000, null_timeout, NULL);
    cycle_set_event(cycle, fde, CYCLE_READ_EVENT, master_process_pipe_callback, NULL);
}

void io_process_pipe_request(cycle_t *cycle, fd_entry_t *fde, void* data) {
    char buf[sizeof(io_op_t *)];
    ssize_t len;
    while ((len = read(fde->fd, buf, sizeof(io_op_t *))) > 0) { 
        io_op_t** addr_of_op = (io_op_t**) buf; 
        io_op_t* op = *addr_of_op;
        assert(len == sizeof(io_op_t *));

        if (!op) {
            continue;
        }
        op->io_cycle = cycle;
        switch (op->type) {
        case IO_READ:
            io_process_get_block(cycle, op);
            break;
        case IO_WRITE:
            io_process_distribute_block(cycle, op);
            break;
        }
    }
    cycle_set_timeout(cycle, &fde->timer, 1000, null_timeout, NULL);
    cycle_set_event(cycle, fde, CYCLE_READ_EVENT,
            io_process_pipe_request, NULL);
}

void main_put_request(io_op_t *op) {
    char *buf = calloc(1, sizeof(io_op_t *));
    memcpy(buf, &op, sizeof(io_op_t *));
    fd_entry_t *fde = master.m_write_fde[op->io_id];
    cycle_set_timeout(op->main_cycle, &fde->timer, 1000, null_timeout, NULL);
    cycle_write(op->main_cycle, fde, buf, sizeof(io_op_t *), sizeof(io_op_t *),
            null_cycle_write_callback, NULL);
}

void io_put_response(cycle_t *cycle, io_op_t *op) {
    io_thread_t *iot = master.iot_group[op->io_id];
    assert(cycle == iot->cycle);
    fd_entry_t *fde = iot->io_pipe_write_fde;
    char *buf = calloc(1, sizeof(io_op_t *));
    memcpy(buf, &op, sizeof(io_op_t *));
    cycle_set_timeout(cycle, &fde->timer, 1000, null_timeout, NULL);
    cycle_write(cycle, fde, buf, sizeof(io_op_t *), sizeof(io_op_t *), null_cycle_write_callback, NULL);
}

void main_distribute_block(cycle_t *cycle, int io_id, int block_id, int fd,
        int offset, int length, block_t *bt, void *callback, void *data)
{
    io_op_t *op = calloc(1, sizeof(io_op_t));
    op->type = IO_WRITE;
    op->fd = fd;
    op->offset = offset;
    op->length = length;
    op->bt = bt;
    op->callback = callback;
    op->data = data;
    op->main_cycle = cycle;
    op->io_id = io_id;
    op->block_id = block_id;
    main_put_request(op);
}

void main_distribute_block_done(cycle_t *cycle, io_op_t *op) {
    void (*cb)(admin_data_t *ad, int status, int block_id) =
            (void (*)(admin_data_t *, int, int))op->callback;
    admin_data_t *ad = op->data;
    cb(ad, op->status, op->block_id);
}

void io_process_distribute_block(cycle_t *cycle, io_op_t *op) {
    int i;
    char *buf = calloc(1, op->length);
    ssize_t cursor = 0;
    while (cursor < op->length) {
        int read_ret = pread(op->fd, buf + cursor, op->length - cursor, op->offset + cursor);
        if (read_ret > 0) {
            cursor += read_ret;
        } else {
            if (errno_ignorable(errno)) {
                continue;
            }
            log(LOG_RUN_ERROR, "pread error\n", op->fd, cursor);
            free(buf);
            io_process_distribute_block_done(op, 0, -1);
            return ;
        }
    }
    log(LOG_DEBUG, "pread fd %d %d bytes\n", op->fd, cursor);
    log(LOG_DEBUG, "offset %d, length %d\n", op->offset, op->length);
    for (i = 0; i < BACK_UP_COUNT; i ++) {
        if (op->bt->store_slave_id[i] != 0) {
            block_t *bt = op->bt;
            unsigned char *key = bt->key;
            char *temp_buf = calloc(1, op->length);
            memcpy(temp_buf, buf, op->length);
            log(LOG_DEBUG, "slave id %hd, length %d, temp_buf %p\n",
                    op->bt->store_slave_id[i], op->length, temp_buf);
            master_distribute_block(cycle, key, op->bt->store_slave_id[i], op->length, temp_buf,
                    io_process_distribute_block_done, op);
        }
    }
    free(buf);
}

void io_process_distribute_block_done(io_op_t *op, int slave_id, int status) {
    int i;
    if (slave_id == 0) {
        log(LOG_RUN_ERROR, "pread file error\n");
        op->status = -1;
        io_put_response(op->io_cycle, op);
        return ;
    }
    if (status == 0) {
        block_mark_slave_available(op->bt, slave_id);
    } else {
        block_mark_slave_unavailable(op->bt, slave_id);
        log(LOG_RUN_ERROR, "slave process error, slave_id %d\n", slave_id);
    }
    int all_succ = 1;
    op->status = -1;
    for (i = 0; i < BACK_UP_COUNT; i ++) {
        //wait the end connection back
        if (op->bt->store_slave_id[i] != 0 && op->bt->store_slave_status[i] == 0) {
            all_succ = 0;
        }
        //only one slave succ, the file store succ
        if (op->bt->store_slave_id[i] != 0 && op->bt->store_slave_status[i] == 1) {
            op->status = 0;
        }
    }
    if (all_succ) {
        io_put_response(op->io_cycle, op);
    }
}

void main_get_block(cycle_t *cycle, int io_id, int block_id, int fd,
        int offset, int length, block_t *bt, void *callback, void *data)
{
    io_op_t *op = calloc(1, sizeof(io_op_t));
    op->type = IO_READ;
    op->fd = fd;
    op->offset = offset;
    op->length = length;
    op->bt = bt;
    op->callback = callback;
    op->data = data;
    op->main_cycle = cycle;
    op->io_id = io_id;
    op->block_id = block_id;
    main_put_request(op);
}

void main_get_block_done(cycle_t *cycle, io_op_t *op) {
    void (*cb)(admin_data_t *ad, int status, int block_id) =
            (void (*)(admin_data_t *, int, int))op->callback;
    admin_data_t *ad = op->data;
    cb(ad, op->status, op->block_id);
}

void io_process_get_block(cycle_t *cycle, io_op_t *op) {
    int i;
    for (i = 0; i < BACK_UP_COUNT; i ++) {
        if (op->bt->store_slave_id[i] != 0 && op->bt->store_slave_status[i] == 1) {
            log(LOG_DEBUG, "slave id %hd, length %d\n",
                    op->bt->store_slave_id[i], op->length);
            master_get_block(cycle, op->bt->key, op->bt->store_slave_id[i], op->length,
                    io_process_get_block_done, op);
            return ;
        }
    }
    log(LOG_RUN_ERROR, "no available slave, %d\n", op->block_id);
    io_process_get_block_done(op, 0, -1);
}

void io_process_get_block_done(io_op_t *op, int slave_id, int status) {
    int i;
    ssize_t cursor = 0;
    int write_ret;
    if (slave_id == 0) {
        log(LOG_RUN_ERROR, "slave id error\n");
        op->status = -1;
        goto FINISH;
    }
    if (status == 0) {
        block_mark_slave_available(op->bt, slave_id);
        goto SUCC;
    } else {
        log(LOG_RUN_ERROR, "slave process error, slave_id %d, try to find backup slave\n", slave_id);
        block_mark_slave_unavailable(op->bt, slave_id);
    }

    for (i = 0; i < BACK_UP_COUNT; i ++) {
        if (op->bt->store_slave_id[i] != 0 && op->bt->store_slave_status[slave_id] == 1) {
            log(LOG_DEBUG, "slave id %hd, length %d\n",
                    op->bt->store_slave_id[i], op->length);
            master_get_block(op->io_cycle, op->bt->key, op->bt->store_slave_id[i], op->length,
                    io_process_get_block_done, op);
            return;
        }
    }
    op->status = -1;
    goto FINISH;
SUCC:
    assert(op->buf);
    while (cursor < op->length) {
        write_ret = pwrite(op->fd, op->buf + cursor, op->length - cursor, op->offset + cursor);
        if (write_ret > 0) {
            cursor += write_ret;
        } else {
            if (errno_ignorable(errno)) {
                continue;
            }
            log(LOG_RUN_ERROR, "pwrite error\n", op->fd, cursor);
            op->status = -1;
            goto FINISH;
        }
    }
    log(LOG_DEBUG, "pwrite fd %d %d bytes\n", op->fd, cursor);
    log(LOG_DEBUG, "offset %d, length %d\n", op->offset, op->length);
    op->status = 0;
FINISH:
    io_put_response(op->io_cycle, op);
}

void block_mark_slave_available(block_t *bt, int slave_id) {
    int i;
    for (i = 0; i < BACK_UP_COUNT; i ++) {
        if (bt->store_slave_id[i] == slave_id) {
            bt->store_slave_status[i] = 1;
        }
        return ;
    }
}
void block_mark_slave_unavailable(block_t *bt, int slave_id) {
    int i;
    for (i = 0; i < BACK_UP_COUNT; i ++) {
        if (bt->store_slave_id[i] == slave_id) {
            bt->store_slave_status[i] = 2;
        }
        return ;
    }
}
