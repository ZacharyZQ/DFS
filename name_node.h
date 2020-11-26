#ifndef __NAME_NODE
#define __NAME_NODE
#include <stdint.h>

#pragma pack(1)
typedef struct block_s {
    unsigned char key[16];
    int16_t store_slave_id[2];
    int16_t part;
    uint8_t padding[2];
    int32_t block_id;
    int32_t next_part_block_id;
} block_t;
#pragma pack()

#pragma pack(1)
typedef struct dir_entry_s {
    char name[32];
} dir_entry_t;
#pragma pack()

#pragma pack(1)
typedef struct file_entry_s {
    char name[32];
    int32_t block_id[8];
} file_entry_t;
#pragma pack()
//0 128 256 384 512 640 768 896

void name_node_init();
int create_dir_name_node(char *path);
int create_file_name_node(char *path);
#endif
