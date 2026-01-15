#include "item_track.h"
#include "utils.h"   // 你已有的 get_time_ns
#include "log.h"

void item_track_on_pop(item_track_t *t, struct queue *q, void* item)
{
    if (!t) return;
    t->last_pop_ts = now_ns();

    LOG("[TRACK] item id=%" PRIu64 ", item=%p queue=%s , queue id=%d, pop",
            t->id, item, q->name, q->id);
}

void item_track_on_push(item_track_t *t, struct queue *q, void *item)
{
//    if (!t || t->last_pop_ts == 0)
//        return;

    uint64_t now = now_ns();
    uint64_t dur = now - t->last_pop_ts;

    LOG("[TRACK] item id=%d, item=%p queue=%s , queue id:%d, push=%lu us",
          t->id,
          item,
          q->name, q->id,
          dur / 1000);

    /* 清零，等待下一轮 */
    t->last_pop_ts = 0;
}

