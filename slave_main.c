#include "dfs_head.h"
char hostname[16];
int main () {
    log_init();
    gethostname(hostname, sizeof(hostname));
    printf("hostname: %s\n", hostname);
    log(LOG_DEBUG, "this is slave\n");
    return 0;
}
