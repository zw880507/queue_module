#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include "utils.h"

int queue_init(queue_t *q, const char *name, int capacity, const item_ops_t *item_ops)
{
    q->ring = calloc(capacity, sizeof(void *));
    if (!q->ring) return -1;

    q->capacity = capacity;
    q->head = q->tail = q->count = 0;
    q->stopped = 0;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);

    q->notify_cb = NULL;
    q->notify_ctx = NULL;
    q->notify_qidx = -1;

    q->monitor = NULL;

    q->item_ops = item_ops;

    if (name) {
        strncpy(q->name, name, sizeof(q->name) - 1);
        q->name[sizeof(q->name) - 1] = '\0';
    } else {
        q->name[0] = '\0';
    }
    return 0;
}

void queue_deinit(queue_t *q)
{
    free(q->ring);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

int queue_push(queue_t *q, void *item)
{
    pthread_mutex_lock(&q->lock);


    if (q->count == q->capacity && q->full_enter_ts == 0) {
        q->full_enter_ts = now_ns();
        if (q->monitor)
            q->monitor(q->monitor_ctx, q,
                          QUEUE_EVENT_FULL_ENTER, 0);
    }

    while (q->count == q->capacity && !q->stopped)
        pthread_cond_wait(&q->not_full, &q->lock);


    if (q->full_enter_ts > 0) {
        uint64_t dt = now_ns() - q->full_enter_ts;
        q->full_enter_ts = 0;
        if (q->monitor)
            q->monitor(q->monitor_ctx, q,
                          QUEUE_EVENT_FULL_LEAVE, dt);
    }

    if (q->stopped) {
        pthread_mutex_unlock(&q->lock);
        return -1;
    }

    q->ring[q->tail] = item;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    pthread_cond_signal(&q->not_empty);

    if (q->notify_cb)
        q->notify_cb(q->notify_ctx, q->notify_qidx);

    pthread_mutex_unlock(&q->lock);
    return 0;
}

void *queue_pop(queue_t *q)
{
    pthread_mutex_lock(&q->lock);

    if (q->count == 0 && q->empty_enter_ts == 0) {
        q->empty_enter_ts = now_ns();
        if (q->monitor)
            q->monitor(q->monitor_ctx, q,
                          QUEUE_EVENT_EMPTY_ENTER, 0);
    }

    while (q->count == 0 && !q->stopped) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    if (q->count == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    if (q->empty_enter_ts > 0) {
        uint64_t dt = now_ns() - q->empty_enter_ts;
        q->empty_enter_ts = 0;
        if (q->monitor)
            q->monitor(q->monitor_ctx, q,
                          QUEUE_EVENT_EMPTY_LEAVE, dt);
    }

    void *item = q->ring[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return item;
}

int queue_try_pop(queue_t *q, void **out)
{
    int ret = -1;
    pthread_mutex_lock(&q->lock);
    if (q->count > 0) {
        *out = q->ring[q->head];
        q->head = (q->head + 1) % q->capacity;
        q->count--;
        pthread_cond_signal(&q->not_full);
        ret = 0;
    }
    pthread_mutex_unlock(&q->lock);
    return ret;
}

void queue_recycle_item(queue_t *q, void *item) {
    queue_push(q, item);
}

void *queue_alloc_item(queue_t *q)
{
    void *item = NULL;

    if (!q || !q->item_ops || !q->item_ops->alloc)
        return NULL;

    pthread_mutex_lock(&q->lock);

    /* 等待有容量 */
    while (q->count == q->capacity && !q->stopped)
        pthread_cond_wait(&q->not_full, &q->lock);

    if (q->stopped) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    /* 占用一个 slot（但不入 ring） */
//    q->count++;

    pthread_mutex_unlock(&q->lock);

    /* 真正创建 item */
    item = q->item_ops->alloc(q->item_ops->ctx, 1);
    if (!item) {
        /* 回滚 count */
        pthread_mutex_lock(&q->lock);
 //       q->count--;
        pthread_cond_signal(&q->not_full);
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    LOG("%s: %s: alloc item %p count=%d", q->name, __func__, item, q->count);
    return item;
}

void queue_free_item(queue_t *q, void *item)
{
    if (!q || !item || !q->item_ops || !q->item_ops->free)
        return;

    q->item_ops->free(item, q->item_ops->ctx);

    pthread_mutex_lock(&q->lock);
//    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    LOG("%s: free item %p count=%d", __func__, item, q->count);
}


void queue_wakeup(queue_t *q)
{
    pthread_mutex_lock(&q->lock);
    q->stopped = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

void queue_set_notify(queue_t *q,
                      void (*cb)(void *ctx, int qidx),
                      void *ctx,
                      int qidx)
{
    q->notify_cb = cb;
    q->notify_ctx = ctx;
    q->notify_qidx = qidx;
}

void queue_set_monitor(queue_t *q,
                       queue_monitor cb,
                       void *ctx)
{
    q->monitor = cb;
    q->monitor_ctx = ctx;
}
