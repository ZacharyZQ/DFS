#ifndef __BFS_H
#define __BFS_H
#include "list.h"
#include "hash.h"

#define B_DISK  ((uint8_t)0x01)
#define B_MEM   ((uint8_t)0x02)

typedef struct {
    int (*init_disk_alloc)(uint64_t total_page);
    int64_t (*page_alloc)(uint64_t size);
    void (*page_free)(uint64_t offset);
    void (*destory_disk_alloc)();
    void *private_data;
} disk_alloc_ops;

typedef struct {
    hash_link hash;
    struct list_head lru;
    unsigned char key[16];
    uint64_t offset;
    char *buf;
    uint32_t length;
    uint8_t flag;
} block_location_t;

typedef struct {
    hash_table_t *block_hash_table;
    unsigned int lru_size;
    unsigned int lru_count;
    struct list_head LRU_head;
} block_manager_t;

typedef struct {
    block_manager_t *bmt;
    disk_alloc_ops *disk_alloc;
    int disk_fd;
    uint64_t disk_size;
} bfs_t;

typedef struct {
    uint64_t total_page_num;
    uint64_t free_page_num;
    rbtree_t *free_space;
    int free_space_size;
    int max_alloc_page;
} default_disk_alloc_data_t;


extern bfs_t *bfs;
extern disk_alloc_ops default_disk_alloc;

block_manager_t *block_manager_init();
block_location_t *init_new_block();
int bfs_init();
block_location_t *init_block(unsigned char key[16], uint64_t offset,
        uint32_t length, char *buf);
block_location_t *get_block(unsigned char key[16]);
void update_lru();
void bfs_write(char *buf, uint32_t length, uint64_t offset);
void bfs_read(char *buf, uint32_t length, uint64_t offset);

#endif
