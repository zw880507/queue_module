#include "sync_module.h"
#include <stdlib.h>
#include <string.h>

static void *sync_thread(void *arg)
{
    sync_module_t *m = arg;

    char tname[16];
    snprintf(tname, sizeof(tname), "sync_module");
    pthread_setname_np(pthread_self(), tname);

    while (m->running) {

        /* 1. 补齐输入 */
        for (int i = 0; i < m->n; ++i) {
            if (!m->inputs[i].item) {
                m->inputs[i].item = queue_pop(m->in_qs[i]);
                if (!m->inputs[i].item)
                    continue;
            }
        }

        /* 2. 是否齐 */
        int ready = 1;
        for (int i = 0; i < m->n; ++i) {
            if (!m->inputs[i].item) {
                ready = 0;
                break;
            }
        }
        if (!ready)
            continue;

        /* 3. policy 决策 */
        int idxs[m->n];
        for (int i = 0; i < m->n; ++i) idxs[i] = i;

        int drop = -1;
        uint64_t diff_ns = 0;
        int ok = m->policy->decide(m, idxs, m->n, m->tolerance_ns, &drop, &diff_ns);

        if (ok == 0) {
            item_group_t *group = m->group_ops->alloc(m, m->n);
            if (!group) {
                /* alloc 失败，直接回收 items */
                for (int i = 0; i < m->n; ++i) {
                    queue_recycle_item(m->recycle_qs[i], m->inputs[i].item);
                    m->inputs[i].item = NULL;
                }
                continue;
            }

            group->count = m->n;
            for (int i = 0; i < m->n; ++i) {
                group->items[i] = m->inputs[i].item;
                m->inputs[i].item = NULL;
            }

            group->recycle_fn = m->group_ops->free;

            queue_push(m->out_q, group);

        } else {
            /* ❌ 丢弃一个 */
//            m->ops->recycle(drop, m->inputs[drop].item);
            queue_recycle_item(m->recycle_qs[drop], m->inputs[drop].item);
            m->inputs[drop].item = NULL;
        }
    }
    return NULL;
}

int sync_module_init(
        sync_module_t *m,
        int n,
        queue_t **in_qs,
        queue_t *out_q,
        queue_t **recycle_qs,
        const sync_policy_t *policy,
        const item_ops_t *item_ops,
        const item_ops_t *group_ops,
        int tolerance_ns)
{
    memset(m, 0, sizeof(*m));
    m->n = n;
    m->in_qs = in_qs;
    m->out_q = out_q;
    m->recycle_qs = recycle_qs;
    m->policy = policy;
    m->item_ops = item_ops;
    m->group_ops = group_ops;
    m->inputs = calloc(n, sizeof(sync_input_t));
    m->running = 1;
    m->tolerance_ns = tolerance_ns;
    return 0;
}

void sync_module_start(sync_module_t *m)
{
    pthread_create(&m->thread, NULL, sync_thread, m);
}

void sync_module_stop(sync_module_t *m)
{
    m->running = 0;
    for (int i = 0; i < m->n; ++i)
        queue_wakeup(m->in_qs[i]);

    pthread_join(m->thread, NULL);
}

