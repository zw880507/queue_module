#pragma once
#include <stdint.h>
#include <stddef.h>
//#include "queue.h"

typedef struct item_ops {
    uint64_t (*get_timestamp_ns)(void *item);
    void *(*alloc)(void *ctx, int count);
    void (*free)(void *item, void *ctx);
    void *ctx;
} item_ops_t;
