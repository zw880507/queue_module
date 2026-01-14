#include "queue_ops.h"
#include "queue.h"

static int q_init(queue_t *q, const char *name, int capacity, const struct item_ops *item_ops) {
    return queue_init(q, name, capacity, item_ops);
}

static void q_deinit(queue_t *q) { queue_deinit(q); }
static int q_push(queue_t *q, void *item) { return queue_push(q, item); }
static int q_push_front(queue_t *q, void *item) { return queue_push_front(q, item); }
static void *q_pop(queue_t *q) { return queue_pop(q); }
static int q_try_pop(queue_t *q, void **out) { return queue_try_pop(q, out); }
static void *q_alloc_item(queue_t *q) { return queue_alloc_item(q); }
static void q_free_item(queue_t *q, void *item) { queue_free_item(q, item); }
static void q_recycle_item(queue_t *q, void *item) { queue_recycle_item(q, item); }
static void q_wakeup(queue_t *q) { queue_wakeup(q); }
static void q_set_notify(queue_t *q, void (*cb)(void*,int), void *ctx, int qidx) {
    queue_set_notify(q, cb, ctx, qidx);
}
static void q_set_monitor(queue_t *q, queue_monitor cb, void *ctx) {
    queue_set_monitor(q, cb, ctx);
}

const queue_ops_t default_queue_ops = {
    .init = q_init,
    .deinit = q_deinit,
    .push = q_push,
    .push_front = q_push_front,
    .pop = q_pop,
    .try_pop = q_try_pop,
    .alloc_item = q_alloc_item,
    .free_item = q_free_item,
    .recycle_item = q_recycle_item,
    .wakeup = q_wakeup,
    .set_notify = q_set_notify,
    .set_monitor = q_set_monitor,
};

