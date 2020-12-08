#include "dfs_head.h"
int check_ms_context(ms_context_t *a) {
    if (a->method < 1 || a->method > 10) {
        log(LOG_RUN_ERROR, "implement method\n");
    }
    if (a->slave_id < 0 || a->slave_id > MAX_SLAVE) {
        log(LOG_RUN_ERROR, "invalid slave id\n");
    }
    return 0;
}


int16_t g_slave_id = 0;
slave_info_t *get_slave_info() {
    slave_info_t *s = calloc(1, sizeof(slave_info_t));
    gethostname(s->slave_name, 16);
    s->slave_id = g_slave_id;
    s->is_online = 1;
    s->total_page_num = 100;
    s->free_page_num = 100;
    return s;
}
