#include "dfs_head.h"

extern dirtree_node_alloc_ops default_alloc_ops;
dir_tree_t *dir_tree_init(int is_rebuild) {
    dir_tree_t *tree = calloc(1, sizeof(dir_tree_t));
    tree->alloc_ops = &default_alloc_ops;
    dirtree_node_t *root = tree->alloc_ops->alloc_root_node();
    root->type = T_DIR;
    list_head_init(&root->child_node);
    list_head_init(&root->sibling_node);
    root->parent_node = NULL;
    memcpy(root->obj.dir_info.name, "/", sizeof("/"));
    return tree;
}

void dir_tree_destory(dir_tree_t * tree) {
    free(tree);
}

void dir_tree_insert(dirtree_node_t *parent, dirtree_node_t *node) {
    if (parent->type != T_DIR) {
        return ;
    }
    node->parent_node = parent;
    list_add(&node->sibling_node, &parent->child_node);
}

void dir_tree_delete(dirtree_node_t *node) {
    node->parent_node = NULL;
    list_del(&node->sibling_node);
}

//not implement
void tree_delete_recursive(dirtree_node_t *node) {
    
}

void default_alloc_init();
dirtree_node_t *default_alloc_root_node();
dirtree_node_t *default_alloc_dirtree_node();
void default_free_dirtree_node(dirtree_node_t *);


dirtree_node_alloc_ops default_alloc_ops = {
    .init = default_alloc_init,
    .alloc_root_node = default_alloc_root_node,
    .alloc_dirtree_node = default_alloc_dirtree_node,
    .free_dirtree_node = default_free_dirtree_node,
};

void default_alloc_init() {
}

dirtree_node_t *default_alloc_root_node() {
    dirtree_node_t *ret = calloc(1, sizeof(dirtree_node_t));
    return ret;
}

dirtree_node_t *default_alloc_dirtree_node() {
    dirtree_node_t *ret = calloc(1, sizeof(dirtree_node_t));
    return ret;
}

void default_free_dirtree_node(dirtree_node_t *n) {
    free(n);
}
