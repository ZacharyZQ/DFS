#ifndef __NAME_NODE
#define __NAME_NODE
#include <stdint.h>

#pragma pack(1)
typedef struct block_s {
    unsigned char key[16];
    int16_t store_slave_id[8];
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
    uint32_t content_length;
    int8_t MR_ops_type;
} file_entry_t;
#pragma pack()

void name_node_init();
int check_path_name_valid(char *path);
int create_dir(char *path);
int create_file(char *path);
int remove_dir(char *path);
int remove_file(char *path);


//the map_reduce_ops is phony Map-Reduce, it can't make slave do some calculation.
//How to improve it?
//I think it needs a mechanism which could make the running process get new code and run it
//and then the master can transimit the new code to slave.
//That is true Map-Reduce
typedef struct {
    int8_t ops_type;
/*
 *  prepare private data for MAP and REDUCE
 */
    void *(*MR_init)();
/*
 *  get next block and put it in buf, 
 *  @private_data:  every file's map reduce private data
 *  @buf:  save the data
 *  @size: the capacity of data
 *  return:  
 *     MR_CONTINUE          
 *     MR_FINISH
 *     MR_FAILED
 */
    int (*MAP)(void *private_data, char *buf, int size);    
/*
 *  it decided how to return the file to user, and it can process data
 *  @private_data:  every file's map reduce private data
 *  @buf:  save the data
 *  @size: the capacity of data
 *  return 
 *     MR_CONTINUE
 *     MR_FINISH
 *     MR_FAILED
 */
    int (*REDUCE)(void *private_data, char *buf, int size);
/*
 *  destory private data
 */
    void (*MR_destory)(void *private_data);
} map_reduce_ops;

map_reduce_ops *g_MR_ops_array[128];



#endif
