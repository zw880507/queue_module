#include <stdlib.h>
#include "item_ops.h"
#include "item_group.h"

static uint64_t group_get_ts(void *item, void* ctx)
{
    item_group_t *g = item;
    return g->timestamp;
}

static void *group_alloc(int count, void *ctx) {
    (void)ctx;
    item_group_t *group = calloc(1, sizeof(item_group_t));
    group->count = count;
    group->items = calloc(count, sizeof(void*));

   return group;
}

static void group_free(void *item, void *ctx) {
    (void)ctx;
    item_group_t *g = item;
    free(g->items);
    free(g);
}

//static 
const item_ops_t group_item_ops = {
    .get_timestamp_ns = group_get_ts,
    .alloc = group_alloc,
    .free = group_free,
    .ctx = NULL,
};

