#pragma once
#include <stdint.h>
#include <stddef.h>
#include "item_track.h"

struct item_track;
typedef struct item_ops {
    uint64_t (*get_timestamp_ns)(void *item, void *ctx);
    struct item_track *(*get_track)(void *item, void *ctx);
    void *(*alloc)(void *ctx, int count);
    void (*free)(void *item, void *ctx);
    void *ctx;
} item_ops_t;
