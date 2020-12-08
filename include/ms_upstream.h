#ifndef __MS_UPSTREAM_H
#define __MS_UPSTREAM_H

struct upstream_data_s;

typedef void OCB(struct upstream_data_s *, int status);

typedef struct upstream_data_s {
    ms_context_t head;
    fd_entry_t *fde;
    void *content_buf;
    void *extra_buf;
    mem_buf_t recv_buf;
    int recv_stage;
    OCB *operation_callback;
    void *callback;
    void *data;
} upstream_data_t;

void slave_register(cycle_t *cycle);
void master_distribute_block(cycle_t *cycle, unsigned char *key, int16_t slave_id, int32_t content_length,
        void *content_buf, void *callback, void *data);
void master_get_block(cycle_t *cycle, unsigned char *key, int16_t slave_id, int32_t content_length,
        void *callback, void *data);
#endif
