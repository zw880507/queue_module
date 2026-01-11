#pragma once

#include <pthread.h>
#include "queue.h"
#include "buffer.h"
#include "context.h"

/* Fill buffer with new data, return 0 on success */
typedef int (*source_fill_fn)(buffer_t *b, void *ctx);

/* Allocate a new buffer */

typedef struct source_module {
    pthread_t          tid;
    int                running;

    queue_t           *in_q;
    queue_t           *out_q;

    source_fill_fn     fill;
    void              *ctx;
} source_module_t;

int source_module_start(source_module_t *m);

int source_module_init(source_module_t *m,
                        queue_t *in_q,
                        queue_t *out_q,
                        source_fill_fn fill,
                        void *ctx);


void source_module_stop(source_module_t *m);

