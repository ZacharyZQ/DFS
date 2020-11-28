#include "dfs_head.h"
int parse_reg_request(void *buf, int16_t extra_data_length, void *data) {
    return 0;
}

int parse_reg_reply(void *buf, int16_t extra_data_length, void *data) {
    return 0;
}

int parse_info_forward(void *buf, int16_t extra_data_length, void *data) {
    return 0;
}

parse_handler_t g_parse_handler_array[] = {
    {REG_REQUEST_TYPE, sizeof(reg_request_t), parse_reg_request},
    {REG_REPLY_TYPE,   sizeof(reg_reply_t),   parse_reg_reply},
    {INFO_FORWARD_TYPE,sizeof(info_forward_t),parse_info_forward}
};

int check_ms_context(ms_context_t * a) {
    return 0;
}
