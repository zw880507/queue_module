#include "process_ops.h"
#include "queue.h"
#include "log.h"

/*
 * 默认 pick：
 *   - 所有非 NULL items 都进入 picked
 */
static pick_result_t default_pick(
        void **items,
        int item_cnt,
        const item_ops_t *item_ops,
        int *picked_idxs,
        int *picked_cnt,
        int *drop_idxs,
        int *drop_cnt,
        int *requeue_idxs,
        int *requeue_cnt,
        void *ctx)
{
    (void)ctx;
    (void)drop_idxs;
    (void)requeue_idxs;
    (void)requeue_cnt;
//    (void)item_ops;

    *picked_cnt  = 0;
    *drop_cnt    = 0;
    *requeue_cnt = 0;

    for (int i = 0; i < item_cnt; ++i) {
        if (items[i])
            picked_idxs[(*picked_cnt)++] = i;
    }

    if (*picked_cnt == 0)
        return PICK_SKIP;


for (int i = 0; i < item_cnt; ++i) {
LOG("%s:%d, item:%p, timestamp:%ld", __func__, __LINE__, items[i], item_ops->get_timestamp_ns(items[i], item_ops->ctx));
}
    return PICK_OK;
}

/*
 * 默认 process：
 *   - 永远成功
 */
static process_result_t default_process(
        void **items,
        int *picked_idxs,
        int picked_cnt,
        void *ctx)
{
    (void)items;
    (void)picked_idxs;
    (void)picked_cnt;
    (void)ctx;
LOG("%s:%d", __func__, __LINE__);
    return 0;
}

static void default_done(void *ctx, int error_code)
{
    (void)ctx;
    (void)error_code;
LOG("%s:%d", __func__, __LINE__);
}

static void default_pop_timeout_ns(void *ctx, uint64_t *timeout)
{
    (void) ctx;
    *timeout = 10 * 1000 * 1000;
}

/* 对外导出的默认 ops */
const process_ops_t default_process_ops = {
    .pick    = default_pick,
    .process = default_process,
    .done    = default_done,
    .pop_timeout_ns = default_pop_timeout_ns,
};

