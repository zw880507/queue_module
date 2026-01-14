#include "queue_module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "log.h"

int pop_multi_with_timeout(queue_module_t *m,
                           queue_thread_map_t *map,
                           uint64_t timeout_ns,
                           void **items,            /* size = n */
                           int *popped_midxs,       /* size = n */
                           int *popped_cnt,
                           int *missed_midxs,       /* size = n */
                           int *missed_cnt)
{
    int n = map->qcount;
    uint64_t deadline = now_ns() + timeout_ns;

    int got[n];
    memset(got, 0, sizeof(got));

    *popped_cnt = 0;
    *missed_cnt = 0;

    /* 初始化 items */
    for (int i = 0; i < n; ++i)
        items[i] = NULL;

    while (now_ns() < deadline) {

        for (int i = 0; i < n; ++i) {
            if (got[i])
                continue;

            int qi = map->qidxs[i];
            if (!m->queue_ops->try_pop(m->in_qs[qi], &items[i])) {
//                items[i] = item;
                got[i]   = 1;
                popped_midxs[(*popped_cnt)++] = i;
            }
        }

        /* 已经全部 pop 到 */
        if (*popped_cnt == n)
            break;

        /* 防止 busy loop */
        struct timespec ts = {
            .tv_sec  = 0,
            .tv_nsec = 1000000, /* 1ms */
        };
        nanosleep(&ts, NULL);
    }

    /* 统计 missed */
    for (int i = 0; i < n; ++i) {
        if (!got[i])
            missed_midxs[(*missed_cnt)++] = i;
    }

    if (*popped_cnt == 0)
        return POP_MULTI_TIMEOUT;

    return POP_MULTI_OK;
}

static void queue_module_run(queue_module_t *m,
                             queue_thread_map_t *map)
{
    int n = map->qcount;
    if (n <= 0 || !m->process_ops)
        return;

    uint64_t pop_timeout_ns = 0;
    if (m->process_ops->pop_timeout_ns)
        m->process_ops->pop_timeout_ns(m->ctx, &pop_timeout_ns);
    void *items[n];
    int popped_midxs[n], missed_midxs[n];
    int popped_cnt = 0, missed_cnt = 0;

    int ret = pop_multi_with_timeout(
                  m,
                  map,
                  pop_timeout_ns,
                  items,
                  popped_midxs,
                  &popped_cnt,
                  missed_midxs,
                  &missed_cnt);

    if (ret == POP_MULTI_TIMEOUT) {
        if (m->process_ops->done)
            m->process_ops->done(m->ctx, PROCESS_POP_TIMEOUT);
        return;
    }

    /* -------- pick 阶段 -------- */

    int picked_midxs[n];
    int drop_midxs[n];
    int requeue_midxs[n];
    int picked_cnt  = 0;
    int drop_cnt    = 0;
    int requeue_cnt = 0;

    pick_result_t pick_ret = PICK_OK;

    if (m->process_ops->pick) {
        pick_ret = m->process_ops->pick(
                        items,
                        n,
                        picked_midxs,  &picked_cnt,
                        drop_midxs,    &drop_cnt,
                        requeue_midxs, &requeue_cnt,
                        m->ctx);
    } else {
        /* 默认 pick：所有已 pop 的 */
        for (int i = 0; i < popped_cnt; ++i)
            picked_midxs[i] = popped_midxs[i];
        picked_cnt = popped_cnt;
    }

    /* -------- drop -------- */
    for (int i = 0; i < drop_cnt; ++i) {
        int midx = drop_midxs[i];
        if (!items[midx])
            continue;

        int qi = map->qidxs[midx];
        m->queue_ops->recycle_item(m->recycle_qs[qi], items[midx]);
        items[midx] = NULL;
    }

    /* -------- requeue (必须 front) -------- */
    for (int i = 0; i < requeue_cnt; ++i) {
        int midx = requeue_midxs[i];
        if (!items[midx])
            continue;

        int qi = map->qidxs[midx];
        m->queue_ops->push_front(m->in_qs[qi], items[midx]);
        items[midx] = NULL;
    }

    if (pick_ret != PICK_OK || picked_cnt == 0) {
        if (m->process_ops->done)
            m->process_ops->done(m->ctx, PROCESS_PICK_SKIP);
        return;
    }

    /* -------- process -------- */

    int handled = 0;
    if (m->process_ops->process) {
        handled = m->process_ops->process(
                      items,
                      picked_midxs,
                      picked_cnt,
                      m->ctx);
    }

    if (handled) {
        /* process 失败 → recycle */
        for (int i = 0; i < picked_cnt; ++i) {
            int midx = picked_midxs[i];
            if (!items[midx])
                continue;

            int qi = map->qidxs[midx];
            m->queue_ops->recycle_item(m->recycle_qs[qi], items[midx]);
            items[midx] = NULL;
        }

        if (m->process_ops->done)
            m->process_ops->done(m->ctx, PROCESS_PROC_ERROR);
    } else {
        /* 正常 forward */
        for (int i = 0; i < picked_cnt; ++i) {
            int midx = picked_midxs[i];
            if (!items[midx])
                continue;

            int qi = map->qidxs[midx];
            m->queue_ops->push(m->out_qs[qi], items[midx]);
            items[midx] = NULL;
        }

        if (m->process_ops->done)
            m->process_ops->done(m->ctx, PROCESS_NO_ERROR);
    }
}


static void *queue_module_thread(void *arg)
{
    queue_thread_ctx_t *tctx = arg;
    queue_module_t *m = tctx->m;
    int tid = tctx->tid;

    queue_thread_map_t *map = &m->thread_maps[tid];

    while (m->running) {
        queue_module_run(m, map);
    }
    return NULL;
}

static int queue_module_balance(queue_module_t *m,
                                int request_thread_count,
                                int *real_thread_count)
{
    int T = request_thread_count;
    int Q = m->inq_count;

    if (T <= 0 || Q <= 0)
        return -1;

    m->thread_maps = calloc(T, sizeof(queue_thread_map_t));
    if (!m->thread_maps)
        return -1;

#if 0
    /* ========== SYNC Mode ========== */
    if (m->process_ops && m->process_ops->sync) {

        m->thread_maps[0].qcount = Q;
        m->thread_maps[0].qidxs  = calloc(Q, sizeof(int));
        if (!m->thread_maps[0].qidxs)
            return -1;

        for (int i = 0; i < Q; ++i)
            m->thread_maps[0].qidxs[i] = i;

        *real_thread_count = 1;
        return 0;
    }
#endif

    /* ========== STREAM Mode ========== */
    int used_threads = (Q < T) ? Q : T;
    *real_thread_count = used_threads;

    int *cnt = calloc(used_threads, sizeof(int));
    if (!cnt)
        return -1;

    for (int i = 0; i < Q; ++i)
        cnt[i % used_threads]++;

    for (int t = 0; t < used_threads; ++t) {
        m->thread_maps[t].qcount = cnt[t];
        if (cnt[t] > 0)
            m->thread_maps[t].qidxs = calloc(cnt[t], sizeof(int));
    }

    memset(cnt, 0, sizeof(int) * used_threads);
    for (int i = 0; i < Q; ++i) {
        int t = i % used_threads;
        m->thread_maps[t].qidxs[cnt[t]++] = i;
    }

    free(cnt);
    return 0;
}

int queue_module_init(queue_module_t *m,
                      const char *name,
                      queue_t **in_qs, int inq_count,
                      queue_t **out_qs, int outq_count,
                      queue_t **recycle_qs, int rcyq_count,
                      const item_ops_t *item_ops,
                      const queue_ops_t *queue_ops,
                      const process_ops_t *process_ops,
                      const thread_ops_t *thread_ops,
                      void *ctx)
{
    if (!m || !queue_ops)
        return -1;

    memset(m, 0, sizeof(*m));
    strncpy(m->name, name ? name : "", sizeof(name) < QM_NAME_SIZE - 1 ? sizeof(name): QM_NAME_SIZE);
    m->name[QM_NAME_SIZE - 1] = '\0';

    m->in_qs       = in_qs;
    m->inq_count   = inq_count;
    m->out_qs      = out_qs;
    m->outq_count  = outq_count;
    m->recycle_qs  = recycle_qs;
    m->rcyq_count  = rcyq_count;

    m->item_ops    = item_ops;
    m->queue_ops   = queue_ops;
    m->process_ops = process_ops;
    m->thread_ops  = thread_ops;
    m->ctx         = ctx;
    m->running     = 1;

    return 0;
}

int queue_module_start(queue_module_t *m)
{
    if (!m)
        return -1;

    char request_thread_name[QM_THREAD_NAME_SIZE] = QM_DEFAULT_THREAD_NAME;

    if (m->thread_ops && m->thread_ops->get_thread_name) {
        m->thread_ops->get_thread_name(&request_thread_name);
    }

    char real_thread_name[QM_THREAD_NAME_SIZE + QM_NAME_SIZE];
    size_t name_len = strlen(m->name);
    size_t request_len = strlen(request_thread_name);

    if (name_len >= sizeof(m->name)) name_len = sizeof(m->name) - 1;
    if (request_len >= sizeof(request_thread_name)) request_len = sizeof(request_thread_name) - 1;

    snprintf(real_thread_name, sizeof(real_thread_name), 
         "%.*s-%.*s", 
         (int)name_len, m->name, 
         (int)request_len, request_thread_name);
    LOG("%s:%d, request_thread_name:%s, m->name:%s, real_name:%s", __func__, __LINE__, request_thread_name, m->name, real_thread_name);

    int request_thread_count = QM_DEFAULT_THREAD_COUNT;
    if (m->thread_ops && m->thread_ops->get_thread_count) {
        m->thread_ops->get_thread_count(&request_thread_count);
    }

    if (request_thread_count <= 0) {
        request_thread_count = QM_DEFAULT_THREAD_COUNT;
    }

    int real_thread_count = 0;
    queue_module_balance(m, request_thread_count, &real_thread_count);

    if (real_thread_count <= 0) {
        real_thread_count = QM_DEFAULT_THREAD_COUNT;
    }

    m->real_thread_count = real_thread_count;

    int request_thread_priority = QM_DEFAULT_THREAD_PRIORITY;
    if (m->thread_ops && m->thread_ops->get_thread_priority) {
        m->thread_ops->get_thread_priority(&request_thread_priority);
    }

    m->threads = calloc(real_thread_count, sizeof(pthread_t));
    if (!m->threads)
        return -1;

    m->thread_ctxs = calloc(real_thread_count, sizeof(queue_thread_ctx_t));

    if (!m->threads || !m->thread_ctxs) {
        free(m->threads);
        m->threads = NULL;
        return -1;
    }

    for (int i = 0; i < real_thread_count; ++i) {

        m->thread_ctxs[i].m   = m;
        m->thread_ctxs[i].tid = i;
        pthread_attr_t attr;
        pthread_attr_init(&attr);


        struct sched_param param = {
            .sched_priority = request_thread_priority
        };
        pthread_attr_setschedparam(&attr, &param);

        int ret = pthread_create(&m->threads[i],
                       &attr,
                       queue_module_thread,
                       &m->thread_ctxs[i]);
        pthread_attr_destroy(&attr);

        if (ret != 0) {
            /* 如果创建失败，join 已创建的线程并清理 */
            for (int j = 0; j < i; ++j)
                pthread_join(m->threads[j], NULL);
            return -1;
        }

        char tname[QM_THREAD_NAME_SIZE +QM_NAME_SIZE];
        snprintf(tname, sizeof(tname), "%s-%d",
                 real_thread_name[0] ?
                    real_thread_name : m->name,
                 i);
        pthread_setname_np(m->threads[i], tname);
    }
    return 0;
}

int queue_module_stop(queue_module_t *m)
{
    if (!m)
        return -1;

    m->running = 0;

    for (int i = 0; i < m->inq_count; ++i)
        m->queue_ops->wakeup(m->in_qs[i]);

    for (int i = 0; i < m->real_thread_count; ++i)
        pthread_join(m->threads[i], NULL);

    if (m->thread_maps) {
        for (int i = 0; i < m->real_thread_count; ++i)
            free(m->thread_maps[i].qidxs);
        free(m->thread_maps);
        m->thread_maps = NULL;
    }

    free(m->threads);
    m->threads = NULL;


    return 0;
}

