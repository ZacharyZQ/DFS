#include "dfs_head.h"
void process_upstream_reply(ud) {
    cycle_close(ud->fde);
    ud->fde = NULL;
    ud->operation_callback(ud, 0);
}

int parse_upstream_reply(upstream_data_t *ud, char *buf, int size) {
    mem_buf_append(&ud->recv_buf, buf, size);
    switch (ud->recv_stage) {
    case RECV_NOTHING:
        if (ud->recv_buf.size >= sizeof(ms_context_t)) {
            memcpy(&ud->head, ud->recv_buf.buf, sizeof(ms_context_t));
            if (check_ms_context(&ud->head) != 0) {
                return -1;
            }
            ud->recv_stage ++;
        } else {
            return 1;         //again read
        }
    case RECV_CONTEXT:
    case RECV_EXTRA:
    case RECV_CONTENT:
        if (sizeof(ms_context_t) + ud->head.extra_length + ud->head.content_length
                < ud->recv_buf.size)
        {
            return -1;
        } else if (sizeof(ms_context_t) + ud->head.extra_length +
                ud->head.content_length == ud->recv_buf.size)
        {
            ud->recv_stage = RECV_DONE;
        } else {
            break;
        }
    case RECV_DONE:
        if (ud->head.extra_length > 0) {
            ud->extra_buf = calloc(1, ud->head.extra_length);
            memcpy(ud->extra_buf, ud->recv_buf.buf + sizeof(ms_context_t),
                    ud->head.extra_length);
        }
        if (ud->head.content_length > 0) {
            ud->content_buf = calloc(1, ud->head.content_length);
            memcpy(ud->content_buf, ud->recv_buf.buf + sizeof(ms_context_t)
                    + ud->head.extra_length, ud->head.content_length);
        }
        return 0;
        break;
    default:
        assert(0);
    }
    return -1;
}

void recv_upstream_reply_timeout(struct cycle_s *cycle, struct timer_s *timer,
        void *data)
{
    upstream_data_t *ud = (upstream_data_t *)data;
    struct fd_entry_s *fde = ud->fde;
    log(LOG_DEBUG, "fd is %d\n", fde->fd);
    cycle_close(cycle, fde);
    ud->fde = NULL;
    ud->operation_callback(ud, -1);
}

void recv_upstream_reply_callback(cycle_t *cycle, fd_entry_t *fde, void* data) {
    upstream_data_t *ud = (upstream_data_t *)data;
    char *buf = calloc(1, 1048576 * 2);
    int size = read(fde->fd, buf, 1048576 * 2);
    //log(LOG_DEBUG, "fd is %d, recv message is %s\n", fde->fd, buf);
    if (size < 0) {
        free(buf);
        if (errno_ignorable(errno)) {
            cycle_set_timeout(cycle, &fde->timer, 5 * 1000,
                    recv_upstream_reply_timeout, ud);
            cycle_set_event(cycle, fde, CYCLE_READ_EVENT, recv_upstream_reply_callback, ud);
            return ;
        } else {
            log(LOG_DEBUG, "read error, close, fd is %d\n", fde->fd);
            cycle_close(cycle, fde);
            ud->fde = NULL;
            ud->operation_callback(ud, -1);
        }
        return ;
    } else if (size == 0) {
        free(buf);
        log(LOG_DEBUG, "close by upstream, fd is %d\n", fde->fd);
        cycle_close(cycle, fde);
        ud->fde = NULL;
        ud->operation_callback(ud, -1);
        return;
    } else {
        int ret = parse_upstream_reply(ud, buf, size);
        free(buf);
        buf = NULL;
        if (ret < 0) {
            log(LOG_RUN_ERROR, "parse_upstream_reply error\n");
            cycle_close(cycle, fde);
            ud->operation_callback(ud, -1);
            ud->fde = NULL;
            return ;
        } else if (ret > 0) {
            cycle_set_timeout(cycle, &fde->timer, 5 * 1000,
                    recv_upstream_reply_timeout, ud);
            cycle_set_event(cycle, fde, CYCLE_READ_EVENT,
                    recv_upstream_reply_callback, ud);
            return ;
        } else {
            process_upstream_reply(ud);
            return ;
        }
    }
}

void send_upstream_request_timeout(struct cycle_s *cycle, struct timer_s *timer,
        void *data)
{
    upstream_data_t *ud = (upstream_data_t *)data;
    log(LOG_DEBUG, "fd is %d\n", upstream_data_t->fde->fd);
    cycle_close(cycle, fde);
    ud->fde = NULL;
    ud->operation_callback(ud, -1);
}

void send_upstream_request_callback(struct cycle_s *cycle, struct fd_entry_s *fde,
        ssize_t size, int status, void* data)
{
    upstream_data_t *ud = (upstream_data_t *)data;
    if (status != CYCLE_OK) {
        log(LOG_DEBUG, "fd is %d\n", fde->fd);
        cycle_close(cycle, fde);
        ud->fde = NULL;
        ud->operation_callback(ud, -1);
    }
    cycle_set_timeout(cycle, &fde->timer, 5 * 1000, recv_upstream_reply_timeout, ud);
    cycle_set_event(cycle, fde, CYCLE_READ_EVENT, recv_upstream_reply_callback, ud);
}

void connect_server_callback(struct cycle_s *cycle, struct fd_entry_s *fde,
        int status, struct in_addr in_addr, int port, void* data)
{
    upstream_data_t *ud = (upstream_data_t *ud)data;
    if (status != CYCLE_OK) {
        log(LOG_DEBUG, "connect error");
        //when connect error, the fde had closed by cycle_connect
        assert(!fde->open);
        ud->fde = NULL;
        ud->operation_callback(ud, -1);
        return;
    }
    log(LOG_DEBUG, "connect success fd is %d\n", fde->fd);
    char *buf = strdup("hello, I am slave\n");
    int size = strlen(buf);
    cycle_set_timeout(cycle, &fde->timer, 5 * 1000, send_upstream_request_timeout, ud);
    cycle_write(cycle, fde, buf, size, 2 * 1048576, send_upstream_request_callback, ud);
}

upstream_data_t *ud_init() {
    upstream_data_t *ud = calloc(1, sizeof(upstream_data_t));
    return ud;
}

void ud_destory(upstream_data_t *ud) {
    if (ud->content_buf) {
        free(ud->content_buf);
    }
    if (ud->extra_buf) {
        free(ud->extra_buf);
    }
    if (ud->send_buf.buf) {
        mem_buf_clean(&ud->send_buf);
    }
    free(ud);
}

void slave_register(char *hostname) {
    upstream_data_t *ud = ud_init();
    ud->head.method = S_METHOD_REG;
    ud->head.slave_id = 0;
    ud->head.extra_length = sizeof(disk_info_t);
    ud->head.extra_data_type = ED_DISK_INFO;
    ud->head.content_length = 0;
    ud->extra_buf = calloc(1, sizeof(disk_info_t));
    ud->operation_callback = slave_register_callback;
    get_disk_info(ud->extra_buf);
    struct in_addr ia;
    fd_entry_t *fde = cycle_open_tcp_nobind(cycle, CYCLE_NONBLOCKING, 0,
            1048576 * 2, 1048576 * 2);
    safe_inet_addr("127.0.0.1", &ia);
    cycle_connect(cycle, fde, ia, 38888, 3, connect_server_callback, ud);
}

void slave_register_callback(upstream_data_t *ud, int status) {
    if (status != 0) {
        log(LOG_RUN_ERROR, "register error\n")
        assert(0);
        return ;
    }
    log(LOG_RUN_ERROR, "register success, the slave id is %hd\n",
            ud->head.slave_id);
    ud_destory(ud);
}
