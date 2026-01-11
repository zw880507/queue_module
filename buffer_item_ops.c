#include <stdlib.h>
#include "item_ops.h"
#include "buffer.h"

static uint64_t buffer_get_ts(void *item)
{
    buffer_t *b = item;
    return b->timestamp;
}

static void *buffer_alloc(void *ctx, int count /* ignore */) {
    (void)ctx;
    return calloc(1, sizeof(buffer_t));
}

static void buffer_free(void *item, void *ctx) {
    (void)ctx;
    buffer_t *b = item;
    free(b);
}

static item_track_t *buffer_get_track(void *item)
{
return NULL;
}

//static 
const item_ops_t buffer_item_ops = {
    .get_timestamp_ns = buffer_get_ts,
    .alloc = buffer_alloc,
    .free = buffer_free,
    .get_track = buffer_get_track,
    .ctx = NULL,
};

