#include "thread_ops.h"
#include "config.h"

static int default_get_thread_name(char **tname) {
    if (*tname) {
        strncpy(tname, QM_THREAD_NAME, sizeof(QM_THREAD_NAME)); 
    }
    return 0;
}

static int default_get_thread_count(int *tcount) {
    if (tcount) {
        *tcount = QM_THREAD_COUNT;
    }
    return 0;
}

static int default_get_thread_priority(int *tpriority) {
    if (tpriority) {
        *tpriority = QM_THREAD_PRIORITY;
    }
    return 0;
}
const thread_ops_t default_thread_ops = {
    .get_thread_name = default_get_thread_name,
    .get_thread_count = default_get_thread_count,
    .get_thread_priority = default_get_thread_priority,
};
