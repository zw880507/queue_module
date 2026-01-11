#include "source_module.h"
#include "log.h"
#include <unistd.h>

static void *thread_fn(void *arg)
{
    source_module_t *m = arg;

    context_t *cxt = (context_t *) m->ctx;
    char tname[16];
    snprintf(tname, sizeof(tname), "source_module_%d", cxt->id);
    pthread_setname_np(pthread_self(), tname);

    while (m->running) {
        /* 1. 从 input queue 获取 buffer（阻塞） */
        void *b = queue_pop(m->in_q);
        if (!b) {
            usleep(1000);
            continue;
        }

        /* 2. 填充数据 */
        if (m->fill(b, m->ctx) != 0) {
            queue_recycle_item(m->in_q, b);  // 填充失败，放回 input queue
            continue;
        }

        LOG("[source] push item %p", b);

        /* 3. 交给下游 */
        queue_push(m->out_q, b);
    }

    return NULL;
}

int source_module_init(source_module_t *m,
                        queue_t *in_q,
                        queue_t *out_q,
                        source_fill_fn fill,
                        void *ctx)
{
    if (!m || !in_q || !out_q || !fill)
        return -1;

    m->in_q   = in_q;
    m->out_q  = out_q;
    m->fill   = fill;
    m->ctx    = ctx;
    m->running = 0;

    return 0;
}

int source_module_start(source_module_t *m)
{
    if (!m)
        return -1;

    m->running = 1;

    return pthread_create(&m->tid, NULL, thread_fn, m);
}


void source_module_stop(source_module_t *m)
{
    if (!m) return;
    m->running = 0;
    pthread_join(m->tid, NULL);
}

