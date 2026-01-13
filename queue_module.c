/* queue_module.c — 修改版 */
#include "queue_module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "log.h"

/* 找到当前线程在 m->threads 中的 index */
static int queue_module_get_tid(queue_module_t *m)
{
    pthread_t self = pthread_self();
    for (int i = 0; i < m->request_thread_count; ++i) {
        if (pthread_equal(self, m->threads[i]))
            return i;
    }
    return -1;
}

static void queue_module_run(queue_module_t *m,
                                  queue_thread_map_t *map)
{

    int n = map->qcount;
    if (n <= 0)
        return;

    if (!m->process_ops)
        return;

    void *items[n * 2];
    int   item_cnt = 0;

    int   out_qidxs[n * 2];

    if (m->process_ops->sync) {
        if (m->process_ops->sync(m->in_qs,
                                 m->recycle_qs,
                                 m->queue_ops,
                                 map->qidxs,
                                 map->qcount,
                                 items,
                                 out_qidxs,
                                 &item_cnt,
                                 m->ctx) != 0) {

            if (m->process_ops->done) {
LOG("%s:%d", __func__, __LINE__);
                m->process_ops->done(m->ctx, PROCESS_SYNC_ERROR);
            }
LOG("%s:%d", __func__, __LINE__);

            return;
        } else if (item_cnt != n) {

LOG("%s:%d", __func__, __LINE__);
            //TODO need recycle here ?
            for (int i = 0; i < item_cnt; ++i) {
                m->queue_ops->recycle_item(m->recycle_qs[out_qidxs[i]], items[i]);
            }

LOG("%s:%d", __func__, __LINE__);
            if (m->process_ops->done) {
LOG("%s:%d", __func__, __LINE__);
                m->process_ops->done(m->ctx, PROCESS_SYNC_ERROR);
            }
LOG("%s:%d", __func__, __LINE__);

            return;
        } else {
LOG("%s:%d, sync OK", __func__, __LINE__);

        }
    } else {
LOG("%s:%d", __func__, __LINE__);
        for (int i = 0; i < n; ++i) {
            int qi = map->qidxs[i];
            items[i] = m->queue_ops->pop(m->in_qs[qi]);
            if (!items[i]) {
                if (m->process_ops->done) {
                    m->process_ops->done(m->ctx, PROCESS_POP_ERROR);
                }

                return;
            }
            out_qidxs[i] = qi;
        }
        item_cnt = n;
LOG("%s:%d", __func__, __LINE__);
    }

    int handled = 0;
    if (m->process_ops->process) {
LOG("%s:%d", __func__, __LINE__);
        handled = m->process_ops->process(items, item_cnt, m->ctx);
LOG("%s:%d", __func__, __LINE__);
    }

    if (handled) {
LOG("%s:%d", __func__, __LINE__);
        for (int i = 0; i < item_cnt; ++i) {
            m->queue_ops->recycle_item(m->recycle_qs[out_qidxs[i]], items[i]);
        }

LOG("%s:%d", __func__, __LINE__);
        if (m->process_ops->done) {
LOG("%s:%d", __func__, __LINE__);
            m->process_ops->done(m->ctx, PROCESS_PROC_ERROR);
LOG("%s:%d", __func__, __LINE__);
        }

        return;

    } else {
LOG("%s:%d", __func__, __LINE__);
        for (int i = 0; i < item_cnt; ++i) {
LOG("%s:%d, out_qidxs i=%d", __func__, __LINE__, out_qidxs[i]);
            m->queue_ops->push(m->out_qs[out_qidxs[i]], items[i]);
        }
LOG("%s:%d", __func__, __LINE__);
    }

    if (m->process_ops->done) {
LOG("%s:%d", __func__, __LINE__);
        m->process_ops->done(m->ctx, PROCESS_NO_ERROR);
LOG("%s:%d", __func__, __LINE__);
    }
}

static void *queue_module_thread(void *arg)
{
    queue_thread_ctx_t *tctx = arg;
    queue_module_t *m = tctx->m;
    int tid = tctx->tid;

    queue_thread_map_t *map = &m->thread_maps[tid];
    char tname[16];
    snprintf(tname, sizeof(tname), "%s-%d",m->request_thread_name, tid);
    pthread_setname_np(pthread_self(), tname);

    while (m->running) {
        queue_module_run(m, map);
    }
    return NULL;
}

static int queue_module_balance(queue_module_t *m,
                                int *real_thread_count)
{
    int T = m->request_thread_count;
    int Q = m->inq_count;

    if (T <= 0 || Q <= 0)
        return -1;

    m->thread_maps = calloc(T, sizeof(queue_thread_map_t));
    if (!m->thread_maps)
        return -1;

    /* ========== SYNC 模式 ========== */
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

    /* ========== STREAM 模式 ========== */
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
                      int request_thread_count,
                      int request_thread_priority,
                      const char *request_thread_name,
                      void *ctx)
{
    if (!m || !queue_ops)
        return -1;

    memset(m, 0, sizeof(*m));
    strncpy(m->name, name ? name : "", NAME_SIZE - 1);
    m->name[NAME_SIZE - 1] = '\0';

    m->in_qs       = in_qs;
    m->inq_count   = inq_count;
    m->out_qs      = out_qs;
    m->outq_count  = outq_count;
    m->recycle_qs  = recycle_qs;
    m->rcyq_count  = rcyq_count;

    m->item_ops    = item_ops;
    m->queue_ops   = queue_ops;
    m->process_ops = process_ops;
    m->ctx         = ctx;
    m->running     = 1;

    m->request_thread_count    = request_thread_count;
    m->request_thread_priority = request_thread_priority;

    if (request_thread_name) {
        /* 直接拷贝固定长度，保证以 '\0' 结尾 */
        strncpy(m->request_thread_name, request_thread_name, NAME_SIZE - 1);
        m->request_thread_name[NAME_SIZE - 1] = '\0';
    } else {
        m->request_thread_name[0] = '\0';
    }

    if (m->request_thread_count <= 0)
        m->request_thread_count = 1;

    m->threads = calloc(m->request_thread_count, sizeof(pthread_t));
    if (!m->threads)
        return -1;

    m->thread_ctxs = calloc(request_thread_count, sizeof(queue_thread_ctx_t));

    if (!m->threads || !m->thread_ctxs) {
        free(m->threads);
        m->threads = NULL;
        return -1;
    }

    return 0;
}

int queue_module_start(queue_module_t *m)
{
    if (!m)
        return -1;

    int real_thread_count = 0;
    if (queue_module_balance(m, &real_thread_count)) {

    LOG("%s:%d, real_thread_count=%d", __func__, __LINE__, real_thread_count);
        return -1;
    }

    LOG("%s:%d, real_thread_count=%d", __func__, __LINE__, real_thread_count);

    for (int i = 0; i < real_thread_count; ++i) {

        m->thread_ctxs[i].m   = m;
        m->thread_ctxs[i].tid = i;
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        if (m->request_thread_priority > 0) {
            struct sched_param param = {
                .sched_priority = m->request_thread_priority
            };
            pthread_attr_setschedparam(&attr, &param);
        }

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

        char tname[NAME_SIZE];
        snprintf(tname, sizeof(tname), "%s-%d",
                 m->request_thread_name[0] ?
                    m->request_thread_name : m->name,
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

    /* 唤醒所有可能在 pop 上阻塞的线程 */
    for (int i = 0; i < m->inq_count; ++i)
        m->queue_ops->wakeup(m->in_qs[i]);

    /* 等待所有线程退出 */
    for (int i = 0; i < m->request_thread_count; ++i)
        pthread_join(m->threads[i], NULL);

    /* 清理 thread_maps 中分配的 qidxs */
    if (m->thread_maps) {
        for (int i = 0; i < m->request_thread_count; ++i)
            free(m->thread_maps[i].qidxs);
        free(m->thread_maps);
        m->thread_maps = NULL;
    }

    free(m->threads);
    m->threads = NULL;

//    free(m->thread_ids);
//    m->thread_ids = NULL;

    return 0;
}

