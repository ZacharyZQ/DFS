#include "dfs_head.h"

dir_tree_t *name_tree;

void name_node_init(int is_rebuild) {
    name_tree = dir_tree_init(is_rebuild);
}

int create_name_node(int type, char *name, dirtree_node_t *parent) {
    //dirtree_node_t *node = name_tree->
    return 0;
}

void *MR_lfs_default_init();
int MR_lfs_default_map(void *private_data, char *buf, int size);
int MR_lfs_default_reduce(void *private_data, char *buf, int size);
void MR_lfs_default_destory(void *private_data);

#define MR_LFS_DEFAULT (int8_t) 0
map_reduce_ops MR_lfs_default_ops = {
    .ops_type = 0,
    .MR_init = MR_lfs_default_init,
    .MAP = MR_lfs_default_map,
    .REDUCE = MR_lfs_default_reduce,
    .MR_destory = MR_lfs_default_destory,
};

void *MR_lfs_default_init() {
    return NULL;
}
int MR_lfs_default_map(void *private_data, char *buf, int size) {
    return 0;
}
int MR_lfs_default_reduce(void *private_data, char *buf, int size) {
    return 0;
}
void MR_lfs_default_destory(void *private_data) {
}
