#include "dfs_head.h"

dir_tree_t *name_tree;

void name_node_init(int is_rebuild) {
    name_tree = dir_tree_init(is_rebuild);
}

int check_path_name_valid(char *path) {
    if (!path || strlen(path) == 0 || path[0] != '/') {
        return 0;
    }
    int len = strlen(path);
    int i;
    for (i = 0; i < len - 1; i ++) {
        if (path[i] == path[i + 1] && path[i] == '/') {
            return 0;
        }
    }
    return 1;
}

int create_dir(char *path) {
    if (!check_path_name_valid(path)) {
        return -1;
    }
    int len = strlen(path);
    char *copy_path;
    if (path[len - 1] == '/') {
        copy_path = strdup(path);
    } else {
        copy_path = calloc(1, len + 2);
        memcpy(copy_path, path, len);
        len ++;
        copy_path[len - 1] = '/';
    }
    int i;
    dirtree_node_t *parent_dir;
    char *sub_dir = calloc(1, len + 2);
    int flag = 0;
    for (i = 0; i < len ; i ++) {
        if (copy_path[i] != '/') {
            continue;
        }
        memset(sub_dir, 0, len + 2);
        memcpy(sub_dir, copy_path, i + 1);
        dirtree_node_t *temp_dir = dir_tree_search(master.tree, sub_dir);
        if (temp_dir) {
            parent_dir = temp_dir;
        } else {
            flag = 1;
            dirtree_node_t *new_node = master.tree->alloc_ops->alloc_root_node();
            new_node->type = T_DIR;
            new_node->parent_node = parent_dir;
            list_head_init(&new_node->child_node);
            list_head_init(&new_node->sibling_node);
            int j;
            for (j = i - 1; j >= 0; j --) {
                if (copy_path[j] == '/') {
                    break;
                } 
            }
            memcpy(new_node->obj.dir_info.name, copy_path + j + 1, i - j);
            dir_tree_insert(parent_dir, new_node);
            parent_dir = new_node;
        }
    }
    free(sub_dir);
    free(copy_path);
    if (flag == 0) {
        return -2;
    }
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
