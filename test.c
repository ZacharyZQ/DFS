#include "dfs_head.h"
#define TEST_RBTREE
//#define MD5_TEST
//#define TEST_LOG
//#define TEST_NETWORK
#define TEST_BFS

void accept_handler_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
}
void read_request_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
    fd_entry_t *fde = (fd_entry_t *)data;
    log(LOG_DEBUG, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
}
void send_reply_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data)
{
    fd_entry_t *fde = (fd_entry_t *)data;
    log(LOG_DEBUG, "fd %d\n", fde->fd);
    cycle_close(cycle, fde);
}
void send_reply_done(cycle_t* cycle, fd_entry_t* fde, ssize_t size,
        int flag, void* data)
{
    log(LOG_DEBUG, "fd %d, write size %ld, flag %d\n", fde->fd, size, flag);
    cycle_close(cycle, fde);
}

void read_request(cycle_t *cycle, fd_entry_t *fde, void* data) {
    char *buf = calloc(1, 1048576 * 2);
    ssize_t size = read(fde->fd, buf, 1048576 * 2);
    if (size < 0) {
        if (errno_ignorable(errno)) {
            cycle_set_timeout(cycle, &fde->timer, 50 * 1000, read_request_timeout, fde);
            cycle_set_event(cycle, fde, CYCLE_READ_EVENT, read_request, NULL);
        } else {
            log(LOG_DEBUG, "read error, close, fd is %d\n", fde->fd);
            cycle_close(cycle, fde);
        }
        return ;
    } else if (size == 0) {
        log(LOG_DEBUG, "close by client, fd is %d\n", fde->fd);
        cycle_close(cycle, fde);
    } else {
        cycle_set_timeout(cycle, &fde->timer, 5 * 1000, send_reply_timeout, fde);
        cycle_write(cycle, fde, buf, size, size, send_reply_done, NULL);
    }
}

void accept_handler(cycle_t *cycle, fd_entry_t *listen_fde, void* data) {
    struct sockaddr_in ain, ame;
    while (1) {
        fd_entry_t *client_fde = cycle_accept(cycle, listen_fde, &ain, &ame);
        if (client_fde == NULL) {
            break;
        }
        log(LOG_DEBUG, "accept connection, listen_fd %d, client fd %d\n",
                listen_fde->fd, client_fde->fd);
        cycle_set_timeout(cycle, &client_fde->timer, 1000, read_request_timeout, client_fde);
        cycle_set_event(cycle, client_fde, CYCLE_READ_EVENT, read_request, NULL);
    }
    cycle_set_timeout(cycle, &listen_fde->timer, 1 * 1000, accept_handler_timeout, NULL);
    cycle_set_event(cycle, listen_fde, CYCLE_READ_EVENT, accept_handler, NULL);
}

void test_network() {
#ifdef TEST_NETWORK
    gettimeofday(&current_time_tv, NULL);
    current_time = current_time_tv.tv_sec;
    cycle_t *cycle = init_cycle();
    struct in_addr ia;
    ia.s_addr = INADDR_ANY;
    int listen_fd = open_listen_fd(
            CYCLE_NONBLOCKING | CYCLE_REUSEADDR | CYCLE_REUSEPORT,
            ia, 38888, 1);
    fd_entry_t *listen_fde = cycle_create_listen_fde(cycle, listen_fd);
    cycle_listen(cycle, listen_fde);
    cycle_set_timeout(cycle, &listen_fde->timer, 1000, accept_handler_timeout, NULL);
    cycle_set_event(cycle, listen_fde, CYCLE_READ_EVENT, accept_handler, NULL);


    
    while (1) {
        struct timeval tv; 
        gettimeofday(&tv, NULL);
        current_time_tv = tv; 
        current_time = current_time_tv.tv_sec;
        LIST_HEAD(accept_events);
        LIST_HEAD(normal_events);
        cycle_wait_post_event(cycle, &accept_events, &normal_events);
        process_posted_events(cycle, &accept_events);
        process_posted_events(cycle, &normal_events);
        cycle_check_timeouts(cycle);
    }
#endif
}

void print_rbtree(rbtree_t *tree) {
    if (rbtree_empty(tree)) {
        return;
    }
    unsigned int key = 0;
    rbtree_node_t *p = rbtree_min(tree->root, tree->sentinel);
    printf("%-5lu  ", p->key);
    key = p->key + 1;
    while (!rbtree_empty(tree)) {
        p = rbtree_succ(tree, key);
        if (!p) {
            printf("\n");
            return ;
        }
        printf("%-5lu\t", p->key);
        key = p->key + 1;
    }
}

int main() {
#ifdef TEST_RBTREE
    printf("test rbtree\n");
    rbtree_t tree;
    rbtree_init(&tree, rbtree_insert_value);
    int i;
    for (i = 0; i < 10; i ++) {
        rbtree_node_t *p = calloc(1, sizeof(rbtree_node_t));
        p->key = rand() % 100;
        rbtree_insert(&tree, p);
    }
    print_rbtree(&tree);
    while (!rbtree_empty(&tree)) {
        rbtree_node_t *p = rbtree_min(tree.root, tree.sentinel);
        printf("%lu\t", p->key);
        rbtree_delete(&tree, p);
        free(p);
    }
    printf("\n");
    printf("\n");
    print_rbtree(&tree);
#endif
#ifdef TEST_BFS
    log_init("log/test.log");
    log(LOG_DEBUG, "hello world\n");
    bfs_init();
    default_disk_alloc_data_t *dad = bfs->disk_alloc->private_data;
    printf("total_page_num: %ld\n", dad->total_page_num);
    printf("free_page_num: %ld\n", dad->free_page_num);
    printf("max_alloc_page: %d\n", dad->max_alloc_page);
    int count = 10;
    while (count > 0) {
        int64_t addr = bfs->disk_alloc->page_alloc(count * 10);
        printf("alloc %d: %ld\n", count * 10, addr);
        for (i = 0; i < dad->free_space_size; i ++) {
            if (!rbtree_empty(&dad->free_space[i])) {
                printf("align: %d\n\t", 1 << i);
            }
            print_rbtree(&dad->free_space[i]);
        }
        count --;
        printf("free_page_num: %ld\n", dad->free_page_num);
    }
#endif
#ifdef MD5_TEST
    printf("test md5\n");
    md5_context M;
    char filename[] = "hello world";
    int part = 100;
    md5_init(&M);
    md5_update(&M, filename, sizeof(filename));
    md5_update(&M, &part, sizeof(part));
    unsigned char key[16];
    md5_final(key, &M);
    char *key_text = calloc(1, 200);
    md5_expand(key, key_text);
    printf("%s", key_text);
    free(key_text);
    printf("\n");
#endif
#ifdef TEST_LOG
    log_init("log/test.log");
    log(LOG_DEBUG, "hello world\n");
#endif
    test_network();   

    return 0;
    
}
