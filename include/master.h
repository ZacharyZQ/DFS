#ifndef __NAME_THREAD_H
#define __NAME_THREAD_H
#include "dir_tree.h"
typedef struct {
    cycle_t *t;
    dir_tree_t *tree;
} master_t;

void master_init();
#endif
