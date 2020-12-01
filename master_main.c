#include "dfs_head.h"

void send_admin_reply_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data);
void send_admin_reply_done(struct cycle_s *cycle, struct fd_entry_s *fde, ssize_t size,
        int flag, void* data);
typedef struct {
    fd_entry_t *fde;
    uint8_t admin_method;
    char *source_file;
    char *dest_file;
    char *reply;
    uint8_t async_callback;
} admin_data_t;

admin_data_t *ad_init(fd_entry_t *fde) {
    admin_data_t *ad = calloc(1, sizeof(admin_data_t));
    ad->fde = fde;
    return ad;
}

void ad_destory(admin_data_t *ad) {
    if (ad->source_file) {
        free(ad->source_file);
    }
    if (ad->dest_file) {
        free(ad->dest_file);
    }
    if (ad->reply) {
        free(ad->reply);
    }
    free(ad);
}

static char* trim_string(char* str) {
    int             len = strlen(str);
    char*           end = str + len - 1; 

    while (isspace(*str) && str <= end) {
        str ++;
    }    

    while (isspace(*end) && str <= end) {
        *end = '\0';
        end --;
    }    
    return str; 
}
#define ADMIN_COMMANDER_LS (uint8_t)1
#define ADMIN_COMMANDER_MKDIR (uint8_t)2
#define ADMIN_COMMANDER_RMDIR (uint8_t)3
#define ADMIN_COMMANDER_RM (uint8_t)4
#define ADMIN_COMMANDER_MVFromLocal (uint8_t)5
#define ADMIN_COMMANDER_MVToLocal (uint8_t)6
#define ADMIN_COMMANDER_STATUS (uint8_t)7
#define ADMIN_COMMANDER_QUERY (uint8_t)8
#define ADMIN_COMMANDER_DATANODE_INFO (uint8_t)9


#define w_space " \t"
int parse_admin_request(admin_data_t *ad, char *buf, int size) {
    char *commander = strdup(buf);
    char* saveptr; 
    char* token = strtok_r(commander, w_space, &saveptr);
    if (!token) {
        ad->reply = strdup("no method\n");
        goto FAILED;
    }
    token = trim_string(token);
    uint8_t method = 0;
    if (!strcmp(token, "ls")) {
        method = ADMIN_COMMANDER_LS;
    } else if (!strcmp(token, "mkdir")) {
        method = ADMIN_COMMANDER_MKDIR;
    } else if (!strcmp(token, "rmdir")) {
        method = ADMIN_COMMANDER_RMDIR;
    } else if (!strcmp(token, "rm")) {
        method = ADMIN_COMMANDER_RM;
    } else if (!strcmp(token, "mvFromLocal")) {
        method = ADMIN_COMMANDER_MVFromLocal;
    } else if (!strcmp(token, "mvToLocal")) {
        method = ADMIN_COMMANDER_MVToLocal;
    } else if (!strcmp(token, "status")) {
        method = ADMIN_COMMANDER_STATUS;
    } else if (!strcmp(token, "query")) {
        method = ADMIN_COMMANDER_QUERY;
    } else if (!strcmp(token, "DataNodeInfo")) {
        method = ADMIN_COMMANDER_DATANODE_INFO;
    }
    if (method == 0) {
        ad->reply = strdup("unrecognize method\n");
        goto FAILED;
    }
    ad->admin_method = method;
    switch(method) {
    //no parameter
    case ADMIN_COMMANDER_STATUS:
    case ADMIN_COMMANDER_DATANODE_INFO:
        break;
    //one parameter
    case ADMIN_COMMANDER_QUERY :
    case ADMIN_COMMANDER_LS    :
    case ADMIN_COMMANDER_MKDIR :
    case ADMIN_COMMANDER_RMDIR :
    case ADMIN_COMMANDER_RM    :
        token = strtok_r(NULL, w_space, &saveptr);
        if (!token) {
            ad->reply = strdup("lack parameter\n");
            goto FAILED;
        }
        token = trim_string(token);
        ad->source_file = strdup(token);
        break;
    //two parameter
    case ADMIN_COMMANDER_MVFromLocal:
    case ADMIN_COMMANDER_MVToLocal:
        token = strtok_r(NULL, w_space, &saveptr);
        if (!token) {
            ad->reply = strdup("lack parameter\n");
            goto FAILED;
        }
        token = trim_string(token);
        ad->source_file = strdup(token);
        token = strtok_r(NULL, w_space, &saveptr);
        if (!token) {
            ad->reply = strdup("lack parameter\n");
            goto FAILED;
        }
        token = trim_string(token);
        ad->source_file = strdup(token);
        break;
    default:
        assert(0);
    }
    token = strtok_r(NULL, w_space, &saveptr);
    if (token) {
        ad->reply = strdup("unnecessary parameter token\n");
        goto FAILED;
    }
    free(commander);
    return 0;
FAILED:
    free(commander);
    return -1;
}

void admin_process_datanode_info(admin_data_t *ad) {
    mem_buf_t mb;
    mem_buf_def_init(&mb);
    dir_tree_printf(master.tree->root, &mb, 1, 1);
    ad->reply = mb.buf;
}

void admin_process_status(admin_data_t *ad) {
    int i;
    mem_buf_t mb;
    mem_buf_def_init(&mb);
    mem_buf_printf(&mb, "DFS\n");
    mem_buf_printf(&mb, "pid: %d\n", getpid());
    mem_buf_printf(&mb, "\n%-30s%-20s%-20s%-20s%-20s\n",
            "slave name", "slave id", "status", "total page num", "free page num");
    for (i = 1; i < MAX_SLAVE; i ++) {
        if (slave_group[i]) {
            mem_buf_printf(&mb, "%-30s%-20hu%-20s%-20u%-20u\n",
                    slave_group[i]->slave_name, slave_group[i]->slave_id,
                    (slave_group[i]->is_online) ? "online" : "offline",
                    slave_group[i]->total_page_num, slave_group[i]->free_page_num);
        }
    }
    ad->reply = mb.buf;
}

void admin_process_query(admin_data_t *ad) {
}

void admin_process_ls(admin_data_t *ad) {
    mem_buf_t mb;
    if (!check_path_name_valid(ad->source_file)) {
        ad->reply = strdup("path name invalid\n");
    }
    dirtree_node_t *node = dir_tree_search(master.tree, ad->source_file);
    if (node == NULL) {
        ad->reply = strdup("path not exist\n");
    } else {
        mem_buf_def_init(&mb);
        dir_tree_printf(node, &mb, 0, 0);
        ad->reply = mb.buf;
    }
}

void admin_process_mkdir(admin_data_t *ad) {
    int ret;
    if ((ret = create_dir(ad->source_file)) == 0) {
        return ;
    }
    switch (ret) {
    case 0:
        return;
    case -1:
        ad->reply = strdup("path name invalid\n");
        return;
    case -2:
        ad->reply = strdup("path existed\n");
        return;
    default:
        break;
    }
    return ;
}

void admin_process_rmdir(admin_data_t *ad) {
}

void admin_process_rm(admin_data_t *ad) {
}

void admin_process_mvFromLocal(admin_data_t *ad) {
}

void admin_process_mvToLocal(admin_data_t *ad) {
}

void process_admin_request(cycle_t *cycle, admin_data_t *ad) {
    switch(ad->admin_method) {
    case ADMIN_COMMANDER_DATANODE_INFO:
        admin_process_datanode_info(ad);
        break;
    case ADMIN_COMMANDER_STATUS:
        admin_process_status(ad);
        break;
    case ADMIN_COMMANDER_QUERY :
        admin_process_query(ad);
        break;
    case ADMIN_COMMANDER_LS    :
        admin_process_ls(ad);
        break;
    case ADMIN_COMMANDER_MKDIR :
        admin_process_mkdir(ad);
        break;
    case ADMIN_COMMANDER_RMDIR :
        admin_process_rmdir(ad);
        break;
    case ADMIN_COMMANDER_RM    :
        admin_process_rm(ad);
        break;
    case ADMIN_COMMANDER_MVFromLocal:
        admin_process_mvFromLocal(ad);
        break;
    case ADMIN_COMMANDER_MVToLocal:
        admin_process_mvToLocal(ad);
        break;
    default:
        assert(0);
    }
    if (ad->async_callback) {
        return;
    }
    if (ad->reply == NULL) {
        cycle_close(cycle, ad->fde);
        ad_destory(ad);
        return ;
    }
    cycle_set_timeout(cycle, &ad->fde->timer, 5 * 1000,
            send_admin_reply_timeout, ad);
    cycle_write(cycle, ad->fde, strdup(ad->reply), strlen(ad->reply),
            2 * 1048576, send_admin_reply_done, ad);
}

void read_admin_request_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
    fd_entry_t *fde = (fd_entry_t *)((char *)timer - offsetof(fd_entry_t, timer));
    log(LOG_DEBUG, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
    if (data) {
        admin_data_t *ad = (admin_data_t *)data;
        ad_destory(ad);
    }
}

void send_admin_reply_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
    fd_entry_t *fde = (fd_entry_t *)((char *)timer - offsetof(fd_entry_t, timer));
    admin_data_t *ad = (admin_data_t *)data;
    assert(fde == ad->fde);
    log(LOG_DEBUG, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
    ad_destory(ad);
}

void send_admin_reply_done(struct cycle_s *cycle, struct fd_entry_s *fde, ssize_t size,
        int flag, void* data)
{
    admin_data_t *ad = (admin_data_t *)data;
    if (flag != CYCLE_OK) {
        log(LOG_RUN_ERROR, "fd %d\n", fde->fd);
    }
    cycle_close(cycle, fde);
    ad_destory(ad);
}

void read_admin_request(cycle_t *cycle, fd_entry_t *fde, void* data) {
    char buf[1048576 * 2] = {0};
    ssize_t size = read(fde->fd, buf, sizeof(buf));
    log(LOG_DEBUG, "read %ld bytes from fd %d\n", size, fde->fd);
    if (size < 0) {
        if (errno_ignorable(errno)) {
            cycle_set_timeout(cycle, &fde->timer, 5 * 1000,
                    read_admin_request_timeout, data);
            cycle_set_event(cycle, fde, CYCLE_READ_EVENT, read_admin_request, data);
        } else {
            log(LOG_DEBUG, "read error, close, fd is %d\n", fde->fd);
            cycle_close(cycle, fde);
            if (data) {
                admin_data_t *ad = (admin_data_t *)data;
                ad_destory(ad);
            }
        }
        return ;
    } else if (size == 0) {
        log(LOG_DEBUG, "close by admin, fd is %d\n", fde->fd);
        cycle_close(cycle, fde);
        if (data) {
            admin_data_t *ad = (admin_data_t *)data;
            ad_destory(ad);
        }
        return;
    } else {
        admin_data_t *ad = (admin_data_t *)data;
        if (ad == NULL) {
            ad = ad_init(fde);
        }
        if (parse_admin_request(ad, buf, size) < 0) {
            log(LOG_DEBUG, "%s, fd is %d\n", ad->reply, fde->fd);
            cycle_set_timeout(cycle, &ad->fde->timer, 5 * 1000,
                    send_admin_reply_timeout, ad);
            cycle_write(cycle, ad->fde, strdup(ad->reply), strlen(ad->reply),
                    2 * 1048576, send_admin_reply_done, ad);
            return ;
        }
        process_admin_request(cycle, ad);
    }
}

void accept_admin_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
}

void accept_admin_handler(cycle_t *cycle, fd_entry_t *listen_fde, void* data) {
    struct sockaddr_in ain, ame;
    while (1) {
        fd_entry_t *admin_fde = cycle_accept(cycle, listen_fde, &ain, &ame);
        if (admin_fde == NULL) {
            break;
        }
        log(LOG_DEBUG, "accept admin connection, admin_fd %d, admin fd %d\n",
                listen_fde->fd, admin_fde->fd);
        cycle_set_timeout(cycle, &admin_fde->timer, 5*1000,
                read_admin_request_timeout, NULL);
        cycle_set_event(cycle, admin_fde, CYCLE_READ_EVENT, read_admin_request, NULL);
    }
    cycle_set_timeout(cycle, &listen_fde->timer, 5 * 1000, accept_admin_timeout, NULL);
    cycle_set_event(cycle, listen_fde, CYCLE_READ_EVENT, accept_admin_handler, NULL);
}


int main () {
    log_init("log/master.log");
    log(LOG_RUN_ERROR, "master start\n");
    master_init();
    cycle_t *cycle = init_cycle();
    struct in_addr ia; 
    ia.s_addr = INADDR_ANY;
    int listen_fd = open_listen_fd(
            CYCLE_NONBLOCKING | CYCLE_REUSEADDR | CYCLE_REUSEPORT,
            ia, 38888, 1); 
    fd_entry_t *listen_fde = cycle_create_listen_fde(cycle, listen_fd);
    cycle_listen(cycle, listen_fde);
    cycle_set_timeout(cycle, &listen_fde->timer, 1000, accept_client_timeout, NULL);
    cycle_set_event(cycle, listen_fde, CYCLE_READ_EVENT, accept_client_handler, NULL);


    int admin_fd = open_listen_fd(
            CYCLE_NONBLOCKING | CYCLE_REUSEADDR,
            ia, 58888, 1); 
    fd_entry_t *admin_fde = cycle_create_listen_fde(cycle, admin_fd);
    cycle_listen(cycle, admin_fde);
    cycle_set_timeout(cycle, &admin_fde->timer, 1000, accept_admin_timeout, NULL);
    cycle_set_event(cycle, admin_fde, CYCLE_READ_EVENT, accept_admin_handler, NULL);
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
