#include "sync_policy.h"
#include "sync_module.h"
#include <stdint.h>

#define TOLERANCE_NS 5000
int sync_policy_window_decide(
        struct sync_module *m,
        int *active_idxs,
        int active_cnt,
        int tolerance_ns,
        int *out_drop_idx,
        uint64_t *out_diff_ns)
{
    if (!m || active_cnt <= 0)
        return -1;

    uint64_t min_ts = 0;
    uint64_t max_ts = 0;
    int min_idx = active_idxs[0];

    for (int k = 0; k < active_cnt; ++k) {
        int i = active_idxs[k];
        void *item = m->inputs[i].item;

        uint64_t ts = m->item_ops->get_timestamp_ns(item, m->item_ops->ctx);

        if (k == 0 || ts < min_ts) {
            min_ts = ts;
            min_idx = i;
        }
        if (k == 0 || ts > max_ts) {
            max_ts = ts;
        }
    }

    uint64_t diff = max_ts - min_ts;
    if (out_diff_ns)
        *out_diff_ns = diff;

//    if (diff <= m->tolerance_ns)
//    if (diff <= m->TOLERANCE_NS)
    if (diff <= tolerance_ns)
        return 0;           /* OK */

    if (out_drop_idx)
        *out_drop_idx = min_idx;

    return -1;              /* drop oldest */
}

const sync_policy_t sync_policy_window = {
    .name   = "window_strict",
    .decide = sync_policy_window_decide,
};

