#ifndef __NAME_THREAD_H
#define __NAME_THREAD_H
#include "dir_tree.h"
typedef struct {
    cycle_t *t;
    dir_tree_t *tree;
} master_t;

typedef struct {
    char slave_name[16];
    int16_t slave_id;
    int8_t is_online;
    uint32_t total_page_num;
    uint32_t free_page_num;
} slave_info_t;

slave_info_t* slave_group[MAX_SLAVE];

void master_init();

extern master_t master;
#endif
