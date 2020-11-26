#ifndef __TREE_H
#define __TREE_H
#include "list.h"
//the end character is /, it represent dir
#define T_DIR 1
#define T_FILE 2

#pragma pack(1)
typedef struct dirtree_node_s {
    struct list_head child_node;
    struct list_head sibling_node;
    struct dirtree_node_s *parent_node;
    int type;
    union {
        dir_entry_t dir_info;
        file_entry_t file_info;
    } obj;
} dirtree_node_t;
#pragma pack()

typedef struct {
    void (*init)();
    dirtree_node_t *(*get_root)();
    dirtree_node_t *(*alloc)();
    void (*free)(dirtree_node_t *);
    void (*sync)();
    void (*destory)();
    int (*rebuild)(dir_tree_t *);
} dirtree_node_alloc_ops;

typedef struct {
    dirtree_node_t *root;
    dirtree_node_alloc_ops alloc_ops;
} dir_tree_t;


dir_tree_t *dir_tree_init(int is_rebuild);
void dir_tree_destory(dir_tree_t *tree);
void dir_tree_insert_to_root(dir_tree_t *tree, dirtree_node_t *node);
void dir_tree_insert(dirtree_node_t *parent, dirtree_node_t *node);
void dir_tree_delete(dirtree_node_t *node);
void dir_tree_delete_recursive(dirtree_node_t *node);

#endif
