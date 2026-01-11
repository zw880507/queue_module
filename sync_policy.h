#ifndef SYNC_POLICY_H
#define SYNC_POLICY_H

#include <stdint.h>

struct sync_module;

typedef int (*policy_decide_fn)(
    struct sync_module *m,
    int *active_idxs,
    int active_cnt,
    int tolerance_ns,
    int *out_drop_idx,
    uint64_t *out_diff_ns);


typedef struct sync_policy {
    const char *name;
    policy_decide_fn decide;
} sync_policy_t;

#endif

