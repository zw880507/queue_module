#pragma once
#include "queue.h"
#include "sync_policy.h"
#include <pthread.h>
#include "item_ops.h"
#include "item_group.h"

typedef struct {
    void *item;
} sync_input_t;

typedef struct sync_module {
    int         n;
    queue_t   **in_qs;
    queue_t    *out_q;
    queue_t   **recycle_qs;

    sync_input_t *inputs;
    const sync_policy_t *policy;
    const item_ops_t    *item_ops;
    const item_ops_t    *group_ops;

    pthread_t thread;
    int       running;

    int tolerance_ns;

} sync_module_t;

int sync_module_init(
        sync_module_t *m,
        int n,
        queue_t **in_qs,
        queue_t *out_q,
        queue_t **recycle_q,
        const sync_policy_t *policy,
        const item_ops_t *ops,
        const item_ops_t *group_ops,
        int tolerance_ns);


void sync_module_start(sync_module_t *m);
void sync_module_stop(sync_module_t *m);

