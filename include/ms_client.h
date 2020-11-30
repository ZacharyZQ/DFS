#ifndef __S2M_H
#define __S2M_H
#define MAX_SLAVE 256
#include "cycle.h"
#include "ms.h"
#define RECV_NOTHING 0
#define RECV_CONTEXT 1
#define RECV_EXTRA 2
#define RECV_CONTENT 3
#define RECV_DONE 4

struct client_data_s {
    ms_context_t head;
    fd_entry_t *fde;
    void *content_buf;
    void *extra_buf;
    mem_buf_t recv_buf;
    int recv_stage;
};

typedef struct client_data_s client_data_t;

void accept_client_timeout(cycle_t *cycle, struct timer_s *timer,
        void* data);
void accept_client_handler(cycle_t *cycle, fd_entry_t *listen_fde, void* data);

#endif
