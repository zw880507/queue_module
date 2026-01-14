#pragma once
#include <stdint.h>
#include "queue.h"
#include "queue_ops.h"


#pragma once
#include <stdint.h>

/*
 * pick / process / done 三阶段模型
 *
 * items[]:
 *   - 长度 = map->qcount
 *   - 与 map index 对齐
 *   - 未 pop 到的数据一定是 NULL
 *
 * 所有 idx 均为 map index（不是 queue index）
 */

/* ---------- 错误码 / 状态码 ---------- */

typedef enum {
    PICK_OK = 0,        /* 正常选取 */
    PICK_SKIP,          /* 本轮不处理（例如等待更多数据） */
    PICK_FATAL,         /* 严重错误，直接放弃本轮 */
} pick_result_t;

typedef enum {
    POP_MULTI_OK = 0,
    POP_MULTI_TIMEOUT,
} pop_result_t;

typedef enum {
    PROCESS_NO_ERROR  = 0,
    PROCESS_POP_TIMEOUT,
    PROCESS_PROC_ERROR,
    PROCESS_PICK_SKIP,
} process_result_t;



/* ---------- process 返回值 ---------- */
/*
 * =0 : 处理成功，forward
 * !=0: 处理失败，recycle
 */
//typedef int process_result_t;

/* ---------- process_ops 定义 ---------- */

typedef struct process_ops {

    /*
     * pick 阶段（只做“选择”，不做 IO）
     *
     * @items:
     *   items[i] 可能为 NULL
     *
     * 输出三组 index（map index）：
     *   picked_idxs   : 进入 process
     *   drop_idxs     : 直接 recycle
     *   requeue_idxs  : 原样塞回 queue（必须 front）
     *
     * 约束：
     *   - 三组 idx 互不重叠
     *   - 只能引用 items 非 NULL 的位置
     */
    pick_result_t (*pick)(
        void   **items,
        int      item_cnt,
        int     *picked_idxs,
        int     *picked_cnt,
        int     *drop_idxs,
        int     *drop_cnt,
        int     *requeue_idxs,
        int     *requeue_cnt,
        void    *ctx);

    /*
     * process 阶段
     *
     * @items:
     *   全量 items 数组
     *
     * @picked_idxs:
     *   pick 阶段给出的 map index
     *
     * 返回：
     *   0  -> forward
     *   !0 -> recycle
     */
    process_result_t (*process)(
        void   **items,
        int     *picked_idxs,
        int      picked_cnt,
        void    *ctx);

    /*
     * done 回调（可选）
     *
     * 用于：
     *   - 统计
     *   - 状态机推进
     *   - 打点
     *
     * error_code 由 queue_module 定义
     */
    void (*done)(
        void *ctx,
        int   error_code);

    void (*pop_timeout_ns) (void *ctx, uint64_t *timeout);


} process_ops_t;

extern const process_ops_t default_process_ops;

