#include <stdlib.h>
#include <string.h>
#include "tracked_item.h"
#include "item_ops.h"
#include "item_track_ops.h"
#include "utils.h"

/*
 * ctx 指向业务 item_ops
 */
static void *item_track_alloc(int count, void *ctx)
{
    item_ops_t *user_ops = ctx;

    tracked_item_t *t = calloc(1, sizeof(*t));
    if (!t)
        return NULL;

    t->track.id = get_next_item_id();

    t->user_item = user_ops->alloc(count, user_ops->ctx);
    if (!t->user_item) {
        free(t);
        return NULL;
    }

//    item_track_init(&t->track);
    return t;
}

static item_track_t *item_track_get_track(void *item, void* ctx)
{
    (void)ctx;
    tracked_item_t *tracked_item = item;
    return &(tracked_item->track);
}


static void item_track_free(void *item, void *ctx)
{
    tracked_item_t *t = item;
    item_ops_t *user_ops = ctx;

    if (!t)
        return;

    user_ops->free(t->user_item, user_ops->ctx);
    free(t);
}

static uint64_t item_track_get_ts(void *item, void* ctx)
{
    item_ops_t *user_ops = ctx;
    tracked_item_t *t = item;
    return user_ops->get_timestamp_ns(t->user_item, user_ops->ctx);
}

void item_track_ops_init(item_ops_t *out,
                           const item_ops_t *user_ops)
{
    memset(out, 0, sizeof(*out));
    out->alloc = item_track_alloc;
    out->free  = item_track_free;
    out->get_timestamp_ns = item_track_get_ts;
    out->get_track = item_track_get_track;
    out->ctx = user_ops;
}

/* not used */
const item_ops_t item_track_ops = {
    .get_timestamp_ns = item_track_get_ts,
    .get_track = item_track_get_track,
    .alloc = item_track_alloc,
    .free = item_track_free,
    .ctx = NULL,
};

