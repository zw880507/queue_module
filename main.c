#include <unistd.h>
#include <stdio.h>

#include "log.h"
#include "queue.h"
#include "sync_module.h"
#include "sync_policy.h"
#include "item_ops.h"
#include "buffer_item_ops.h"
#include "group_item_ops.h"
#include "context.h"
#include "utils.h"
#include "tracked_item.h"
#include "queue_module.h"
#include "thread_ops.h"

/* ================= 配置 ================= */

#define N           30
#define QUEUE_CAP   20

#define QUEUE_COUNT 3

extern const sync_policy_t sync_policy_window;

/* ================= demo source fill ================= */

uint64_t buffer_now_ns() {
    return now_ns();
}

static void _queue_monitor(void *ctx,
                          queue_t *q,
                          queue_event_t ev,
                          uint64_t ns)
{
    switch (ev) {
    case QUEUE_EVENT_FULL_LEAVE:
        if (ns > 1 * 1000 * 1000)
            LOG("%s:%d, queue %s backpressure %.2f ms", __func__, __LINE__,
                q->name, ns / 1e6);
        break;

    case QUEUE_EVENT_EMPTY_LEAVE:
        if (ns > 1 * 1000 * 1000)
            LOG("%s:%d, queue %s starvation %.2f ms", __func__, __LINE__,
                q->name, ns / 1e6);
        break;
    default:
//        LOG("%s:%d, event:%d", __func__, __LINE__, ev);
        break;
    }
}

static void item_latency_cb(
        void *ctx,
        queue_t *q,
        void *item,
        uint64_t ns)
{
    LOG("[queue %s] item %p latency %lu us",
        q->name, item, ns / 1000);
}

/* ================= main ================= */

int main(void)
{
    LOG("demo start");

    /* ---------- queues ---------- */

    queue_t empty_q[N];
    queue_t fill_q[N];
    queue_t *empty_qs[N];
    queue_t *fill_qs[N];

    queue_t process_q[N];
    queue_t *process_qs[N];

    item_ops_t tracked_item_ops; 
    item_track_ops_init(&tracked_item_ops, &buffer_item_ops);

    for (int i = 0; i < N; i++) {
        char empty_name[50];
        sprintf(empty_name, "empty_queue-%d", i);
        char fill_name[50];
        sprintf(fill_name, "fill_queue-%d", i);

        char process_name[50];
        sprintf(process_name, "process_queue-%d", i);



        queue_init(&empty_q[i],
                   empty_name,
                   QUEUE_CAP * QUEUE_COUNT,
                   &tracked_item_ops);
        queue_init(&fill_q[i],
                   fill_name,
                   QUEUE_CAP,
                   &tracked_item_ops);
//                   &buffer_item_ops);

        queue_init(&process_q[i],
                   process_name,
                   QUEUE_CAP,
                   &tracked_item_ops);


        queue_set_monitor(&empty_q[i], _queue_monitor, NULL);
        queue_set_monitor(&fill_q[i], _queue_monitor, NULL);

        empty_qs[i] = &empty_q[i];
        fill_qs[i] = &fill_q[i];
        process_qs[i] = &process_q[i];

        for (int j = 0; j < QUEUE_CAP * QUEUE_COUNT; j++) {
            void *item = queue_alloc_item(&empty_q[i]);
            queue_push(&empty_q[i], item);
        }

    }

    queue_module_t module1, module2, module3;
    queue_module_init(&module1 /* queue_module_t */,
                      "module1" /* name */,
                      &empty_qs /* in queues */,
                      N /* in queues count */,
                      &fill_qs /* out queues */,
                      N /* out queues count */,
                      &empty_qs /* recycle queues */,
                      N /* recycle queue count */,
                      &tracked_item_ops /* item_ops */,
                      &default_queue_ops /* queue_ops */,
                      &default_process_ops /* process_ops */,
                      &default_thread_ops /* process_ops */,
                      NULL /* ctx */);

    queue_module_start(&module1);

    queue_module_init(&module2 /* queue_module_t */,
                      "module2" /* name */,
                      &fill_qs /* in queues */,
                      N /* in queues count */,
                      &process_qs /* out queues */,
                      N /* out queues count */,
                      &empty_qs /* recycle queues */,
                      N /* recycle queue count */,
                      &tracked_item_ops /* item_ops */,
                      &default_queue_ops /* queue_ops */,
                      &default_process_ops /* process_ops */,
                      &default_thread_ops /* thread_ops */,
                      NULL /* ctx */);

    queue_module_start(&module2);


    queue_module_init(&module3 /* queue_module_t */,
                      "module3" /* name */,
                      &process_qs /* in queues */,
                      N /* in queues count */,
                      &empty_qs /* out queues */,
                      N /* out queues count */,
                      &empty_qs /* recycle queues */,
                      N /* recycle queue count */,
                      &tracked_item_ops /* item_ops */,
                      &default_queue_ops /* queue_ops */,
                      &default_process_ops /* process_ops */,
                      &default_thread_ops /* thread_ops */,
                      NULL /* ctx */);

    queue_module_start(&module3);


while (1)
    sleep(1);


    /* ---------- sync ---------- */

    queue_t sync_out_q;

    queue_init(&sync_out_q,
               "sync_out_queue",
               QUEUE_CAP,
               &buffer_item_ops);

    queue_set_monitor(&sync_out_q, _queue_monitor, NULL);

    sync_module_t sync;

    sync_module_init(&sync,
                     N,
                     fill_qs,
                     &sync_out_q,
                     empty_qs,
                     &sync_policy_window,
                     &tracked_item_ops,
                     &group_item_ops,
                     10000000);

    sync_module_start(&sync);


    while (1) {
        sleep(1);
    }

    /* ---------- cleanup ---------- */

    sync_module_stop(&sync);

    return 0;
}

