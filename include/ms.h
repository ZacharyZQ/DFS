#ifndef __MS_H_
#define __MS_H_
#include <stdint.h>
#define M_METHOD_CREATE ((uint16_t)1)
#define M_METHOD_DELETE ((uint16_t)2)
#define M_METHOD_QUERY  ((uint16_t)3)
#define S_METHOD_REG    ((uint16_t)4)
#define S_METHOD_INFO   ((uint16_t)5)
#define S_METHOD_UNREG  ((uint16_t)6)
#define METHOD_ACK      ((uint16_t)7)

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
    int16_t content_length;
    int64_t seq_num;
    unsigned char padding[2];
    unsigned char key[16];
};
#pragma pack()
typedef struct ms_context_s ms_context_t;
int check_ms_context(ms_context_t *);

typedef int EDP(void *buf, int16_t extra_data_length, void *data);

#define REG_REQUEST_TYPE 1
typedef struct reg_request_s {
    char slave_name[16];
    uint32_t free_block_num;
    uint32_t free_extra_page_num;
    uint32_t capacity_page_num;
    uint32_t type;
} reg_request_t;
int parse_reg_request(void *buf, int16_t extra_data_length, void *data);

#define REG_REPLY_TYPE 2
typedef struct reg_reply_s {
    int16_t status;
    int16_t server_id;
} reg_reply_t;
int parse_reg_reply(void *buf, int16_t extra_data_length, void *data);

#define INFO_FORWARD_TYPE 3
typedef struct info_forward_s {
} info_forward_t;
int parse_info_forward(void *buf, int16_t extra_data_length, void *data);

typedef struct {
   int16_t extra_data_type;
   int16_t extra_data_length;
   EDP *parse_handler;
} parse_handler_t;

#endif
