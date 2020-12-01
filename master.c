#include "dfs_head.h"
master_t master;
void master_init() {
    master.tree = dir_tree_init(1);
}

