#pragma once
#include <stdatomic.h>
static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

static inline uint64_t get_next_queue_id(void)
{
    static atomic_uint_fast64_t qid = 0;
    return atomic_fetch_add(&qid, 1) + 1;
}

static inline uint64_t get_next_item_id(void)
{
    static atomic_uint_fast64_t iid = 0;
    return atomic_fetch_add(&iid, 1) + 1;
}

