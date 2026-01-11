#pragma once
#include <pthread.h>
#include <stdatomic.h>
#include "item_ops.h"
#include "log.h"

struct item_ops;
struct queue;
typedef struct queue queue_t;
typedef enum {
    QUEUE_EVENT_FULL_ENTER = 3,
    QUEUE_EVENT_FULL_LEAVE = 5,
    QUEUE_EVENT_EMPTY_ENTER = 7,
    QUEUE_EVENT_EMPTY_LEAVE = 9,
} queue_event_t;

typedef void (*queue_monitor)(
        void *ctx,
        queue_t *q,
        queue_event_t event,
        uint64_t duration_ns);

typedef struct queue_track {
    uint64_t start_ts;     /* 出发时间 */
    uint64_t start_qid;    /* 出发 queue */
    int      valid;        /* 是否在计时中 */
} queue_track_t;

typedef void (*queue_item_monitor)(
        void *ctx,
        queue_t *q,
        void *item,
        uint64_t duration_ns);

#define ITEM_TRACK(item) \
    ((queue_track_t *)((char *)(item) - sizeof(queue_track_t)))

typedef struct queue {
    uint64_t        id;
    char            name[50];
    void            **ring;
    int             capacity;
    int             head;
    int             tail;
    int             count;

    pthread_mutex_t lock;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;

    /* optional notify */
    void (*notify_cb)(void *ctx, int qidx);
    void  *notify_ctx;
    int    notify_qidx;

    int     stopped;

    const struct item_ops *item_ops;

    /* monitor */
    queue_monitor monitor;
    void *monitor_ctx;
    uint64_t full_enter_ts;
    uint64_t empty_enter_ts;
} queue_t;

/* lifecycle */
int  queue_init(queue_t *q, const char *name, int capacity, const struct item_ops *item_ops);
void queue_deinit(queue_t *q);

/* ops */
int   queue_push(queue_t *q, void *item);     /* blocking */
int   queue_try_pop(queue_t *q, void **out);  /* non-blocking */
void *queue_pop(queue_t *q);                  /* blocking */

/* control */
void queue_wakeup(queue_t *q);

/* notify */
void queue_set_notify(queue_t *q,
                      void (*cb)(void *ctx, int qidx),
                      void *ctx,
                      int qidx);

void queue_recycle_item(queue_t *q, void *item);

/* 申请一个 item（占用 queue 容量，但不入队） */
void *queue_alloc_item(queue_t *q);

/* 释放一个 item（归还容量） */
void queue_free_item(queue_t *q, void *item);

void queue_set_monitor(queue_t *q,
                       queue_monitor cb,
                       void *ctx);
