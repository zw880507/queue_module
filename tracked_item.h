#pragma once
#include <stdint.h>
#include "item_track.h"

typedef struct tracked_item {
    item_track_t track;
    void *user_item;
} tracked_item_t;

