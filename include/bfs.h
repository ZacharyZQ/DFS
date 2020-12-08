#ifndef __BFS_H
#define __BFS_H
#include "list.h"
#include "hash.h"
typedef struct {
    hash_table_t *block_hash_table;
    unsigned int lru_size;
    unsigned int lru_count;
    struct list_head LRU_head;
} block_manager_t;

block_manager_t *block_manager_init(HASHCMP *, int, HASHHASH *, int);



#endif
