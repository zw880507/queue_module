#pragma once
#include "process_ops.h"
#include "queue.h"

static int p_sync(queue_t **in_qs,
                 queue_t **recycle_qs,
                 queue_ops_t *q_ops,
                 int *qidxs,
                 int qcount,
                 void **items,
                 int *out_qidxs,
                 int *out_cnt,
                 void *ctx) {


    *out_cnt = qcount;
    *out_qidxs = *qidxs;

    int n = qcount;

    for (int i = 0; i < n; ++i) {
        int qi = qidxs[i];
        items[i] = q_ops->pop(in_qs[qi]);
        out_qidxs[i] = qi;
    }

LOG("%s:%d qcount:%d, qid:%d", __func__, __LINE__, qcount, qidxs[0]);
    return 0;
}

static int p_process(void **items, int count, void *ctx) {

LOG("%s:%d, count=%d", __func__, __LINE__, count);
    return 0;

}

static void p_done(void *ctx, int error) {

LOG("%s:%d, error code=%d", __func__, __LINE__, error);
}

const process_ops_t default_process_ops = {
//    .sync = p_sync,
    .sync = NULL,
    .process = p_process,
    .done = p_done,
};

