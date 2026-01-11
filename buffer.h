#pragma once
#include <stdint.h>
#include <stdatomic.h>

typedef struct buffer {
    atomic_int refcnt;
    uint64_t   timestamp;
    void      *data;

    /* 由 buffer_queue 使用，不对外 */
    void      *priv;
} buffer_t;

/* 引用管理（不做回收） */
static inline void buffer_ref(buffer_t *b)
{
    atomic_fetch_add_explicit(&b->refcnt, 1, memory_order_relaxed);
}

static inline int buffer_unref(buffer_t *b)
{
    return atomic_fetch_sub_explicit(&b->refcnt, 1, memory_order_acq_rel) - 1;
}

