#pragma once
#include "queue.h"
#include <pthread.h>
#include "item_ops.h"
#include "queue_ops.h"
#include "process_ops.h"
#include "thread_ops.h"
#include "config.h"

typedef int (*queue_module_process_fn)(void *item, void *ctx);
typedef int (*queue_module_done_fn)(void *item, void *ctx);
typedef int (*queue_module_sync_fn)(void **items, int count, void *ctx);

typedef struct queue_thread_map {
    int *qidxs;
    int qcount;
} queue_thread_map_t;

struct queue_module;
typedef struct queue_thread_ctx {
    struct queue_module *m;
    int tid;
} queue_thread_ctx_t;

typedef struct queue_module {
    int       id;
    char      name[QM_NAME_SIZE];
    queue_t   **in_qs;
    queue_t   **out_qs;
    queue_t   **recycle_qs;
    int       inq_count;
    int       outq_count;
    int       rcyq_count;

    pthread_t *threads;
    queue_thread_ctx_t *thread_ctxs;
    queue_thread_map_t *thread_maps;

    void      *ctx;

    const item_ops_t     *item_ops;
    const queue_ops_t    *queue_ops;
    const process_ops_t  *process_ops;
    const thread_ops_t   *thread_ops;

    int real_thread_count;

    int       running;

} queue_module_t;

int queue_module_init(queue_module_t *m,
                      const char *name,
                      queue_t **in_qs, int inq_count,
                      queue_t **out_qs, int outq_count,
                      queue_t **recycle_qs, int rcyq_count,
                      const item_ops_t *item_ops,
                      const queue_ops_t *queue_ops,
                      const process_ops_t *process_ops,
                      const thread_ops_t *thread_ops,
                      void *ctx);

int queue_module_start(queue_module_t *m);
int queue_module_stop(queue_module_t *m);

