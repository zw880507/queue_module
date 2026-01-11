#include "process_module.h"
#include <stdlib.h>


static void process_done_cb(item_group_t *group, void *user)
{
    process_module_t *m = user;

    LOG("%s:%d", __func__, __LINE__);
    for (int i = 0; i < group->count; i++) {
        queue_recycle_item(m->recycle_qs[i], group->items[i]);
    }

    item_group_recycle(group);
}

static void *process_thread(void *arg)
{
    process_module_t *m = arg;

    char tname[16];
    snprintf(tname, sizeof(tname), "process_module");
    pthread_setname_np(pthread_self(), tname);


    while (m->running) {
        item_group_t *group = queue_pop(m->in_q);
        if (!group)
            continue;

        m->process(group, m->ctx, process_done_cb, m);
    }
    return NULL;
}

int process_module_init(
        process_module_t *m,
        queue_t *in_q,
        queue_t **recycle_qs,
        process_fn fn,
        void *ctx)
{
    m->in_q    = in_q;
    m->recycle_qs  = recycle_qs;
    m->process = fn;
    m->ctx     = ctx;
    m->running = 0;

    return 0;
}

int process_module_start(process_module_t *m)
{
    m->running = 1;

    return pthread_create(&m->thread, NULL, process_thread, m);
}

void process_module_stop(process_module_t *m)
{
    m->running = 0;
    queue_wakeup(m->in_q);
    pthread_join(m->thread, NULL);
}

