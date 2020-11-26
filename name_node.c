#include "dsf_head.h"

dir_tree_t *name_tree;

void name_node_init(int is_rebuild) {
    name_tree = dir_tree_init(is_rebuild);
}

static int create_name_node(int type, char *name, dirtree_node_t) {
}

