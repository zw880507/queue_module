#pragma once
#include <stdint.h>
#include "queue.h"

typedef struct item_track {
    uint64_t id;
    uint64_t last_pop_ts;
} item_track_t;

void item_track_on_pop(item_track_t *t, struct queue *q, void *item);
void item_track_on_push(item_track_t *t, struct queue *q, void *item);

