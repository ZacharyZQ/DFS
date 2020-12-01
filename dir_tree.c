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
    tree->root = root;
    return tree;
}

void dir_tree_destory(dir_tree_t * tree) {
    free(tree);
}

void dir_tree_insert(dirtree_node_t *parent, dirtree_node_t *node) {
    node->parent_node = parent;
    list_add(&node->sibling_node, &parent->child_node);
    log(LOG_DEBUG, "parent name %s %p, name %s %p\n",
            parent->obj.dir_info.name, parent, node->obj.dir_info.name, node);
    //if (node->type == T_FILE) {
    //
    //}
}

void dir_tree_delete(dir_tree_t *tree, dirtree_node_t *node) {
    //if (node->type != T_DIR) {
    //need notify slave
    //}

    node->parent_node = NULL;
    list_del(&node->sibling_node);
    if (list_empty(&node->child_node)) {
        tree->alloc_ops->free_dirtree_node(node);
        return ;
    }
    struct list_head *p;
    list_for_each_clear(p, &node->child_node) {
        dirtree_node_t *child = list_entry(p, dirtree_node_t, sibling_node);
        dir_tree_delete(tree, child);
    }
    assert(list_empty(&node->child_node));
    tree->alloc_ops->free_dirtree_node(node);
}

void print_node_path(dirtree_node_t *node, mem_buf_t *mem_buf) {
    if (node->parent_node == NULL) {
        mem_buf_printf(mem_buf, "%s", node->obj.dir_info.name);
        return ;
    }
    print_node_path(node->parent_node, mem_buf);
    if (node->type == T_DIR) {
        mem_buf_printf(mem_buf, "%s", node->obj.dir_info.name);
    } else {
        mem_buf_printf(mem_buf, "%s", node->obj.file_info.name);
    }
}

void dir_tree_printf(dirtree_node_t *node, mem_buf_t *mem_buf,
        int print_self, int recursion)
{
    if (print_self) {
        print_node_path(node, mem_buf);
        mem_buf_printf(mem_buf, "\n");
    }
    struct list_head *p_list_head;
    if (recursion) {
        list_for_each(p_list_head, &node->child_node) {
            dirtree_node_t *temp = list_entry(p_list_head, dirtree_node_t,
                    sibling_node);
            dir_tree_printf(temp, mem_buf, 1, 1);
        }
    } else {
        list_for_each(p_list_head, &node->child_node) {
            dirtree_node_t *temp = list_entry(p_list_head, dirtree_node_t,
                    sibling_node);
            print_node_path(temp, mem_buf);
            mem_buf_printf(mem_buf, "\n");
        }
    }
}

dirtree_node_t *dir_tree_search(dir_tree_t *tree, char *path) {
    if (!check_path_name_valid(path)) {
        return NULL;
    }
    dirtree_node_t *ret = NULL;
    int len = strlen(path);
    char *copy_path = strdup(path);
    char *saveptr;
    char *root_dir = strtok_r(copy_path, "/", &saveptr);
    char *p;
    int flag = 0;
    struct list_head *p_list_head; 
    dirtree_node_t *parent_node;
    dirtree_node_t *temp;
    char *temp_path = calloc(1, len + 5);
    if (root_dir == NULL) {
        ret = tree->root;
        goto FINISH;
    }
    memcpy(temp_path, root_dir, strlen(root_dir));
    temp_path[strlen(root_dir)] = '/';
    parent_node = tree->root;
    list_for_each(p_list_head, &parent_node->child_node) {
        temp = list_entry(p_list_head, dirtree_node_t, sibling_node);
        if (temp->type == T_DIR && !strcmp(temp->obj.dir_info.name, temp_path)) {
            parent_node = temp;
            flag = 1;
            break;
        }
    }
    if (flag == 0) {
        goto FINISH;
    }
    flag = 0;
    while ((p = strtok_r(NULL, "/", &saveptr)) != NULL) {
        memset(temp_path, 0, len + 5);
        memcpy(temp_path, p, strlen(p));
        temp_path[strlen(p)] = '/';
        list_for_each(p_list_head, &parent_node->child_node) {
            temp = list_entry(p_list_head, dirtree_node_t, sibling_node);
            if (temp->type == T_DIR && !strcmp(temp->obj.dir_info.name, temp_path)) {
                parent_node = temp;
                flag = 1;
                break;
            }
        }
        if (flag == 0) {
            goto FINISH;
        }
        flag = 0;
    }
    ret = parent_node;
FINISH:
    free(temp_path);
    free(copy_path);
    return ret;
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
