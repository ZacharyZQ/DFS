#include "dfs_head.h"
void client_data_destory(client_data_t *cd);

void slave_process_get_block(client_data_t *cd) {
    char *file_name = calloc(1, 200);
    strcpy(file_name, "./disk/");
    char *key_text = file_name + strlen("./disk/");
    md5_expand(cd->head.key, key_text);
    log(LOG_DEBUG, "key text is %s, filename is %s\n", key_text, file_name);
    int fd = open(file_name, O_RDONLY);
    int file_size;
    int cursor = 0;
    if (fd < 0) {
        log(LOG_RUN_ERROR, "filename %s open failed\n", file_name);
        goto FAILED;
    }
    if (cd->content_buf) {
        free(cd->content_buf);
        cd->content_buf = NULL;
    }
    file_size = lseek(fd, 0, SEEK_END);
    cd->head.content_length = file_size;
    cd->content_buf = calloc(1, file_size);
    while (cursor < file_size) {
        int pread_ret = pread(fd, cd->content_buf + cursor, file_size - cursor, 0 + cursor);
        if (pread_ret <= 0) {
            if (errno_ignorable(errno)) {
                continue;
            }
            goto FAILED;
        } else {
            cursor += pread_ret;
        }
    }
    cd->head.method = METHOD_ACK_SUCC;
    if (cd->extra_buf) {
        free(cd->extra_buf);
        cd->extra_buf = NULL;
    }
    cd->head.extra_length = 0;
    log(LOG_DEBUG, "process succ %s\n", file_name);
    free(file_name);
    return;
FAILED:
    cd->head.method = METHOD_ACK_FAILED;
    cd->head.content_length = 0;
    cd->head.extra_length = 0;
    if (cd->content_buf) {
        free(cd->content_buf);
        cd->content_buf = NULL;
    }
    if (cd->extra_buf) {
        free(cd->extra_buf);
        cd->extra_buf = NULL;
    }
    if (fd > 0) {
        close(fd);
    }
    free(file_name);
}

void slave_process_create_block(client_data_t *cd) {
    char *file_name = calloc(1, 200);
    strcpy(file_name, "./disk/");
    char *key_text = file_name + strlen("./disk/");
    md5_expand(cd->head.key, key_text);
    log(LOG_DEBUG, "key text is %s, filename is %s\n", key_text, file_name);
    FILE *fp = fopen(file_name, "w+");
    if (!fp) {
        log(LOG_RUN_ERROR, "filename %s open failed\n", file_name);
        cd->head.method = METHOD_ACK_FAILED;
        cd->head.content_length = 0;
        free(cd->content_buf);
        cd->content_buf = NULL;
        if (cd->extra_buf) {
            free(cd->extra_buf);
            cd->extra_buf = NULL;
        }
        cd->head.extra_length = 0;
        free(file_name);
        return ;
    }
    fwrite(cd->content_buf, cd->head.content_length, 1, fp);
    cd->head.method = METHOD_ACK_SUCC;
    cd->head.content_length = 0;
    free(cd->content_buf);
    cd->content_buf = NULL;
    if (cd->extra_buf) {
        free(cd->extra_buf);
        cd->extra_buf = NULL;
    }
    cd->head.extra_length = 0;
    fclose(fp);
    log(LOG_DEBUG, "process succ %s\n", file_name);
    free(file_name);
}

void master_process_slave_register(client_data_t *cd) {
    int i = 0;
    for (i = 1; i < MAX_SLAVE; i ++) {
        if (slave_group[i] == NULL) {
            break;
        }
    }
    if (i == MAX_SLAVE) {
        log(LOG_RUN_ERROR, "Too many slave register in master, register error\n");
        return ;
    }
    slave_info_t *d = (slave_info_t *)cd->extra_buf;
    d->slave_id = (int16_t)i;
    cd->extra_buf = NULL;
    cd->head.extra_length = 0;
    cd->head.method = METHOD_ACK_SUCC;
    cd->head.slave_id = (int16_t)i;
    slave_group[i] = d;
    assert(!cd->content_buf);
    assert(!cd->head.content_length);
    log(LOG_RUN_ERROR, "the alloc slave id is %hd, slave name is %s\n",
            d->slave_id, d->slave_name);
}

void master_process_slave_unregister(client_data_t *cd) {

}

void send_client_reply_done(struct cycle_s *cycle, struct fd_entry_s *fde, ssize_t size, 
                    int flag, void* data)
{
    client_data_t *cd = (client_data_t *)data;
    if (flag != CYCLE_OK) {
        log(LOG_RUN_ERROR, "fd %d\n", fde->fd);
    }
    cycle_close(cycle, fde);
    client_data_destory(cd);
}
void send_client_reply_timeout(cycle_t *cycle, struct timer_s *timer, void* data) {
    client_data_t *cd = (client_data_t *)data;
    fd_entry_t *fde = cd->fde;
    log(LOG_RUN_ERROR, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
    client_data_destory(cd);
}

void process_client_request(cycle_t *cycle, client_data_t *cd) {
    switch (cd->head.method) {
    case M_METHOD_CREATE:
        slave_process_create_block(cd);
        break;
    case M_METHOD_GET:
        slave_process_get_block(cd);
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
    assert(cd->head.method == METHOD_ACK_SUCC || cd->head.method == METHOD_ACK_FAILED);
    char *buf = calloc(1, 2 * 1048576);
    memcpy(buf, &cd->head, sizeof(ms_context_t));
    if (cd->head.extra_length != 0) {
        memcpy(buf + sizeof(ms_context_t), cd->extra_buf, cd->head.extra_length);
    }
    if (cd->head.content_length != 0) {
        memcpy(buf + sizeof(ms_context_t) + cd->head.extra_length,
                cd->content_buf, cd->head.content_length);
    }
    int size = sizeof(ms_context_t) + cd->head.extra_length + cd->head.content_length;
    cycle_set_timeout(cycle, &cd->fde->timer, 5 * 1000, send_client_reply_timeout, cd);
    cycle_write(cycle, cd->fde, buf, size, 2 * 1048576, send_client_reply_done, cd);
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
    log(LOG_DEBUG, "recv size %ld\n", size);
    mem_buf_append(&cd->recv_buf, buf, size);
    switch (cd->recv_stage) {
    case RECV_NOTHING:
        if (cd->recv_buf.size >= sizeof(ms_context_t)) {
            memcpy(&cd->head, cd->recv_buf.buf, sizeof(ms_context_t));
            if (check_ms_context(&cd->head) != 0) {
                return -1;
            }
            log(LOG_DEBUG, "parse head succ, method %hu extra_length %hu content_length %d\n",
                    cd->head.method, cd->head.extra_length, cd->head.content_length);
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
            return 1;
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
    fd_entry_t *fde = (fd_entry_t *)((char *)timer - offsetof(fd_entry_t, timer));
    log(LOG_DEBUG, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
    if (data != NULL) {
        client_data_t *cd = (client_data_t *)data;
        client_data_destory(cd);
    }
}

void read_client_request(cycle_t *cycle, fd_entry_t *fde, void* data) {
    char buf[1048576 * 2];
    ssize_t size = read(fde->fd, buf, sizeof(buf));
    log(LOG_DEBUG, "read %ld bytes from fd %d\n", size, fde->fd);
    if (size < 0) {
        if (errno_ignorable(errno)) {
            cycle_set_timeout(cycle, &fde->timer, 5 * 1000,
                    read_client_request_timeout, data);
            cycle_set_event(cycle, fde, CYCLE_READ_EVENT, read_client_request, data);
        } else {
            log(LOG_DEBUG, "read error, close, fd is %d\n", fde->fd);
            cycle_close(cycle, fde);
            if (data != NULL) {
                client_data_t *cd = (client_data_t *)data;
                client_data_destory(cd);
            }
        }
        return ;
    } else if (size == 0) {
        log(LOG_DEBUG, "close by client, fd is %d\n", fde->fd);
        cycle_close(cycle, fde);
        if (data != NULL) {
            client_data_t *cd = (client_data_t *)data;
            client_data_destory(cd);
        }
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
            log(LOG_DEBUG, "parse succ, method %hu extra_length %hu content_length %d\n",
                    cd->head.method, cd->head.extra_length, cd->head.content_length);
            process_client_request(cycle, cd);
            return ;
        }
    }
}

void accept_client_handler(cycle_t *cycle, fd_entry_t *listen_fde, void* data) {
    struct sockaddr_in ain, ame;
    while (1) {
        fd_entry_t *client_fde = cycle_accept(cycle, listen_fde, &ain, &ame);
        if (client_fde == NULL) {
            break;
        }
        log(LOG_DEBUG, "accept connection, listen_fd %d, client fd %d\n",
                listen_fde->fd, client_fde->fd);
        cycle_set_timeout(cycle, &client_fde->timer, 5*1000,
                read_client_request_timeout, NULL);
        cycle_set_event(cycle, client_fde, CYCLE_READ_EVENT, read_client_request, NULL);
    }
    cycle_set_timeout(cycle, &listen_fde->timer, 5 * 1000, accept_client_timeout, NULL);
    cycle_set_event(cycle, listen_fde, CYCLE_READ_EVENT, accept_client_handler, NULL);
}
