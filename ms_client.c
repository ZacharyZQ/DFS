#include "dfs_head.h"
int process_client_request(client_data_t *cd) {
    switch (cd->head.method) {
    case M_METHOD_CREATE:
        break;
    case M_METHOD_DELETE:
    case M_METHOD_QUERY:
    case S_METHOD_REG:
        master_process_slave_register(cd);
        break;
    case S_METHOD_INFO:
    case S_METHOD_UNREG:
        master_process_slave_unregister(cd);
    default:
        assert(0);
    }
    return 0;
}

client_data_t *client_data_init(fd_entry_t *fde) {
    client_data_t *cd = calloc(1, sizeof(client_data_t));
    cd->fde = fde;
    mem_buf_def_init(&cd->recv_buf);
    return cd;
}

void client_data_destory(client_data_t *cd) {
    if (cd->content_buf) {
        free(cd->content_buf);
    }
    if (cd->extra_buf) {
        free(cd->extra_buf);
    }
    if (cd->recv_buf.buf) {
        mem_buf_clean(&cd->recv_buf);
    }
    free(cd);
}

int parse_client_request(client_data_t *cd, char *buf, ssize_t size) {
    mem_buf_append(&cd->recv_buf, buf, size);
    switch (cd->recv_stage) {
    case RECV_NOTHING:
        if (cd->recv_buf.size >= sizeof(ms_context_t)) {
            memcpy(&cd->head, cd->recv_buf.buf, sizeof(ms_context_t));
            if (check_ms_context(&cd->head) != 0) {
                return -1;
            }
            cd->recv_stage ++;
        } else {
            return 1;         //again read
        }
    case RECV_CONTEXT:
    case RECV_EXTRA:
    case RECV_CONTENT:
        if (sizeof(ms_context_t) + cd->head.extra_length + cd->head.content_length
                < cd->recv_buf.size)
        {
            return -1;
        } else if (sizeof(ms_context_t) + cd->head.extra_length +
                cd->head.content_length == cd->recv_buf.size)
        {
            cd->recv_stage = RECV_DONE;
        } else {
            break;
        }
    case RECV_DONE:
        if (cd->head.extra_length > 0) {
            cd->extra_buf = calloc(1, cd->head.extra_length);
            memcpy(cd->extra_buf, cd->recv_buf.buf + sizeof(ms_context_t),
                    cd->head.extra_length);
        }
        if (cd->head.content_length > 0) {
            cd->content_buf = calloc(1, cd->head.content_length);
            memcpy(cd->content_buf, cd->recv_buf.buf + sizeof(ms_context_t)
                    + cd->head.extra_length, cd->head.content_length);
        }
        return 0;
        break;
    default:
        assert(0);
    }
    return -1;
}

void accept_client_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
}
void read_client_request_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
    fd_entry_t *fde = (fd_entry_t *)data;
    log(LOG_DEBUG, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
}
void send_client_reply_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
    fd_entry_t *fde = (fd_entry_t *)data;
    log(LOG_DEBUG, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
}
void send_client_reply_done(cycle_t* cycle, fd_entry_t* fde, ssize_t size,
        int flag, void* data)
{
    log(LOG_DEBUG, "fd %d, write size %ld, flag %d\n", fde->fd, size, flag);
    cycle_close(cycle, fde);
}

void read_client_request(cycle_t *cycle, fd_entry_t *fde, void* data) {
    char buf[1048576 * 2];
    ssize_t size = read(fde->fd, buf, sizeof(buf));
    if (size < 0) {
        if (errno_ignorable(errno)) {
            cycle_set_timeout(cycle, &fde->timer, 5 * 1000,
                    read_client_request_timeout, fde);
            cycle_set_event(cycle, fde, CYCLE_READ_EVENT, read_client_request, NULL);
        } else {
            log(LOG_DEBUG, "read error, close, fd is %d\n", fde->fd);
            cycle_close(cycle, fde);
        }
        return ;
    } else if (size == 0) {
        log(LOG_DEBUG, "close by client, fd is %d\n", fde->fd);
        cycle_close(cycle, fde);
        return;
    } else {
        client_data_t *cd = NULL;
        if (data == NULL) {
            cd = client_data_init(fde);
        } else {
            cd = (client_data_t *)data;
        }
        int ret = parse_client_request(cd, buf, size);
        if (ret < 0) {
            log(LOG_RUN_ERROR, "parse_client_request error\n");
            cycle_close(cycle, fde);
            client_data_destory(cd);
            cd = NULL;
            return ;
        } else if (ret > 0) {
            cycle_set_timeout(cycle, &fde->timer, 5 * 1000,
                    read_client_request_timeout, cd);
            cycle_set_event(cycle, fde, CYCLE_READ_EVENT,
                    read_client_request, cd);
            return ;
        } else {
            process_client_request(cd);
            return ;
        }
        //cycle_set_timeout(cycle, &fde->timer, 5 * 1000, send_reply_timeout, fde);
        //cycle_write(cycle, fde, buf, size, size, send_reply_done, NULL);
    }
}

void accept_client_handler(cycle_t *cycle, fd_entry_t *listen_fde, void* data) {
    struct sockaddr_in ain, ame;
    while (1) {
        fd_entry_t *client_fde = cycle_accept(cycle, listen_fde, &ain, &ame);
        if (client_fde == NULL) {
            break;
        }
        log(LOG_DEBUG, "accept slave connection, listen_fd %d, client fd %d\n",
                listen_fde->fd, client_fde->fd);
        cycle_set_timeout(cycle, &client_fde->timer, 5*1000,
                read_client_request_timeout, client_fde);
        cycle_set_event(cycle, client_fde, CYCLE_READ_EVENT, read_client_request, NULL);
    }
    cycle_set_timeout(cycle, &listen_fde->timer, 5 * 1000, accept_client_timeout, NULL);
    cycle_set_event(cycle, listen_fde, CYCLE_READ_EVENT, accept_client_handler, NULL);
}
