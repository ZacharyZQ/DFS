#include "dfs_head.h"
char* slave_name[MAX_SLAVE];

int main () {
    log_init("log/master.log");
    log(LOG_DEBUG, "this is master\n");
    return 0;
}
