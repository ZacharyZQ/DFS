#ifndef _RBTREE_H__
#define _RBTREE_H__

typedef unsigned long rbtree_key_t;
typedef int rbtree_key_int_t;
typedef struct rbtree_node_s rbtree_node_t;


#pragma pack(1)
struct rbtree_node_s {
    rbtree_key_t       key;
    rbtree_node_t*     left;
    rbtree_node_t*     right;
    rbtree_node_t*     parent;
    unsigned char      color;
};
#pragma pack()

typedef struct rbtree_s  rbtree_t;

typedef void (*rbtree_insert_pt)(rbtree_node_t* root,
        rbtree_node_t* node, rbtree_node_t* sentinel);

struct rbtree_s {
    rbtree_node_t* root;
    rbtree_node_t* sentinel;
    rbtree_node_t _sentinel;
    rbtree_insert_pt insert;
};


#define rbtree_init(tree, i)                                           \
    rbtree_sentinel_init(&(tree)->_sentinel);                                              \
(tree)->root = &(tree)->_sentinel;                                                         \
(tree)->sentinel = &(tree)->_sentinel;                                                     \
(tree)->insert = i

#define rbtree_empty(tree) \
    ((tree)->root == (tree)->sentinel)


void rbtree_insert(rbtree_t* tree, rbtree_node_t* node);
void rbtree_delete(rbtree_t* tree, rbtree_node_t* node);
void rbtree_insert_value(rbtree_node_t* root, rbtree_node_t* node,
        rbtree_node_t* sentinel);
void rbtree_insert_timer_value(rbtree_node_t* root,
        rbtree_node_t* node, rbtree_node_t* sentinel);


#define rbt_red(node)               ((node)->color = 1)
#define rbt_black(node)             ((node)->color = 0)
#define rbt_is_red(node)            ((node)->color)
#define rbt_is_black(node)          (!rbt_is_red(node))
#define rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */

#define rbtree_sentinel_init(node)  rbt_black(node)

static inline rbtree_node_t*
rbtree_min(rbtree_node_t* node, rbtree_node_t* sentinel) {
    while (node && node->left != sentinel) {
        node = node->left;
    }
    return node;
}

rbtree_node_t *rbtree_prec(rbtree_t *tree, rbtree_key_t key);
rbtree_node_t *rbtree_succ(rbtree_t *tree, rbtree_key_t key);

#endif
