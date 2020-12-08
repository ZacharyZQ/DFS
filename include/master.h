#ifndef __NAME_THREAD_H
#define __NAME_THREAD_H
#include "dir_tree.h"
#include "io_thread.h"
#define IO_THREAD_NUM 8
typedef struct {
    cycle_t *cycle;
    dir_tree_t *tree;
    hash_table_t *block_hash_table;
    fd_entry_t *m_read_fde[IO_THREAD_NUM];
    fd_entry_t *m_write_fde[IO_THREAD_NUM];
    io_thread_t *iot_group[IO_THREAD_NUM];
} master_t;

typedef struct {
    fd_entry_t *fde;
    uint8_t admin_method;
    char *source_file;
    char *dest_file;
    char *reply;
    uint8_t async_callback;
    int8_t *map_block;
    int block_num;
    int fd;
    FILE *fp;
} admin_data_t;

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
