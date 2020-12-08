#include "dfs_head.h"
master_t master;
void master_init() {
    master.tree = dir_tree_init(1);
    master.block_hash_table = hash_create(md5_cmp, 9999991, hash_md5key);
}

