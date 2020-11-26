#include "dir_tree.h"
#include <stdlib.h>
dir_tree_t *dir_tree_init() {
    dir_tree_t *tree = calloc(1, sizeof(dir_tree_t));
    dirtree_node_t *root = &tree->root;
    root->type = 1;
    list_head_init(&root->child_node);
    list_head_init(&root->sibling_node);
    root->parent_node = NULL;
    
    return tree;
}

void dir_tree_destory(dir_tree_t * tree) {
    free(tree);
}

void dir_tree_insert(dirtree_node_t *parent, dirtree_node_t *node) {
    if (!parent->is_dir) {
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
