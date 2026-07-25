// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

#define OSP2_SYNC_OK 0UL
#define OSP2_SYNC_CPU_COUNT_ERROR 1UL
#define OSP2_SYNC_SHARED_TRACE_ERROR 2UL
#define OSP2_SYNC_NO_CONTENTION_ERROR 3UL
#define OSP2_SYNC_LOCK_HELD_ERROR 4UL
#define OSP2_SYNC_ORDER_ERROR 5UL

osp2_u64 osp2_sync_validate(
    osp2_u64 cpu_count,
    osp2_u64 shared_updates,
    osp2_u64 ap_attempts,
    osp2_u64 contention,
    osp2_u64 lock_state,
    osp2_u64 irq_progress
) {
    if (cpu_count != 2) {
        return OSP2_SYNC_CPU_COUNT_ERROR;
    }
    if (shared_updates != 2 || ap_attempts != 1) {
        return OSP2_SYNC_SHARED_TRACE_ERROR;
    }
    if (contention == 0) {
        return OSP2_SYNC_NO_CONTENTION_ERROR;
    }
    if (lock_state != 0) {
        return OSP2_SYNC_LOCK_HELD_ERROR;
    }
    if (irq_progress != 1) {
        return OSP2_SYNC_SHARED_TRACE_ERROR;
    }
    return OSP2_SYNC_OK;
}

osp2_u64 osp2_sync_lock_order(
    osp2_u64 held_rank,
    osp2_u64 requested_rank
) {
    if (requested_rank == 0 || requested_rank > 3) {
        return OSP2_SYNC_ORDER_ERROR;
    }
    if (held_rank != 0 && requested_rank <= held_rank) {
        return OSP2_SYNC_ORDER_ERROR;
    }
    return OSP2_SYNC_OK;
}
