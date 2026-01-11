// item_group.h
#pragma once

#include <stddef.h>

/* 泛型 item group */
typedef struct item_group {
    uint64_t timestamp;
    void **items;    /* array of pointers to items */
    int    count;      /* number of valid items */
    void (*recycle_fn)(void *group, void *ctx);
    int   *idxs;     /* optional: original indexes of items */
} item_group_t;

static inline void item_group_recycle(item_group_t *group) {
    if (group && group->recycle_fn)
        group->recycle_fn(group, NULL);
}

