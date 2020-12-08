#ifndef __MS_H_
#define __MS_H_
#include <stdint.h>
#include "master.h"
#define M_METHOD_CREATE ((uint16_t)1)
#define M_METHOD_GET    ((uint16_t)9)
#define M_METHOD_DELETE ((uint16_t)2)
#define M_METHOD_QUERY  ((uint16_t)3)
#define S_METHOD_REG    ((uint16_t)4)
#define S_METHOD_INFO   ((uint16_t)5)
#define S_METHOD_UNREG  ((uint16_t)6)
#define METHOD_ACK_SUCC ((uint16_t)7)
#define METHOD_ACK_FAILED ((uint16_t)8)

#define PAGE_SIZE  (1 << 12)
#define BLOCK_SIZE (1 << 20)

#if 0
    |   ms_context_t |
    |   extra_data   |
    |   content_data |
#endif

#pragma pack(1)
struct ms_context_s {
    uint16_t method;
    int16_t slave_id;
    int16_t extra_length;
    int16_t extra_data_type;
    int32_t content_length;
    unsigned char key[16];
};
#pragma pack()
typedef struct ms_context_s ms_context_t;
int check_ms_context(ms_context_t *);
slave_info_t *get_slave_info();
#endif
