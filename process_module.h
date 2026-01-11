#pragma once
#include "queue.h"
#include "item_group.h"

typedef void (*process_done_fn)(item_group_t *group, void *user);

typedef void (*process_fn)(
    item_group_t *group,
    void *ctx,
    process_done_fn done,
    void *done_ctx
);

typedef struct process_module {
    queue_t     *in_q;
    queue_t     **recycle_qs;
    pthread_t    thread;
    int          running;

    process_fn   process;
    void        *ctx;
} process_module_t;

int process_module_init(
        process_module_t *m,
        queue_t *in_q,
        queue_t **recycle_qs,
        process_fn fn,
        void *ctx);

int process_module_start(process_module_t *m);

void process_module_stop(process_module_t *m);

