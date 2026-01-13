#pragma once
#include <stdint.h>
#include "queue.h"

typedef struct queue_ops {

    int  (*init)(queue_t *q, const char *name, int capacity, const struct item_ops *item_ops);
    void (*deinit)(queue_t *q);

    int   (*push)(queue_t *q, void *item);        /* blocking push */
    void * (*pop)(queue_t *q);                    /* blocking pop */
    int   (*try_pop)(queue_t *q, void **out);     /* non-blocking pop */

    void *(*alloc_item)(queue_t *q);             /* 申请 item */
    void  (*free_item)(queue_t *q, void *item);  /* 释放 item */
    void  (*recycle_item)(queue_t *q, void *item);

    void (*wakeup)(queue_t *q);

    void (*set_notify)(queue_t *q,
                       void (*cb)(void *ctx, int qidx),
                       void *ctx,
                       int qidx);

    void (*set_monitor)(queue_t *q,
                        queue_monitor cb,
                        void *ctx);

} queue_ops_t;

extern const queue_ops_t default_queue_ops;

