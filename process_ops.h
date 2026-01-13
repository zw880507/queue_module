#pragma once
#include <stdint.h>
#include "queue.h"
#include "queue_ops.h"

typedef struct process_ops {

    int  (*sync)(queue_t **in_qs,
                 queue_t **recycle_qs,
                 queue_ops_t *q_ops,
                 int *qidxs,
                 int qcount,
                 void **items,
                 int *out_qidxs,
                 int *out_cnt,
                 void *ctx);
    int (*process)(void **items, int count, void *ctx);
    void (*done)(void *ctx, int error);

} process_ops_t;

extern const process_ops_t default_process_ops;

