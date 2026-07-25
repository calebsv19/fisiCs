// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_SYNC_RANK_VECTORS_H
#define OSP2_SYNC_RANK_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_SYNC_OK 0UL
#define OSP2_SYNC_CPU_COUNT_ERROR 1UL
#define OSP2_SYNC_SHARED_TRACE_ERROR 2UL
#define OSP2_SYNC_NO_CONTENTION_ERROR 3UL
#define OSP2_SYNC_LOCK_HELD_ERROR 4UL
#define OSP2_SYNC_ORDER_ERROR 5UL
#define OSP2_SYNC_VECTOR_COUNT 51UL
#define OSP2_SYNC_CORPUS_ID "sync51-v1"

osp2_u64 osp2_sync_validate(
    osp2_u64 cpu_count,
    osp2_u64 shared_updates,
    osp2_u64 ap_attempts,
    osp2_u64 contention,
    osp2_u64 lock_state,
    osp2_u64 irq_progress
);
osp2_u64 osp2_sync_lock_order(
    osp2_u64 held_rank,
    osp2_u64 requested_rank
);

static osp2_u64 osp2_sync_expect(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_sync_run_vectors(void) {
    osp2_u64 failures = 0;

    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, 0, 1), OSP2_SYNC_OK
    );

    failures += osp2_sync_expect(
        osp2_sync_validate(0, 2, 1, 1, 0, 1), OSP2_SYNC_CPU_COUNT_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(1, 2, 1, 1, 0, 1), OSP2_SYNC_CPU_COUNT_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(3, 2, 1, 1, 0, 1), OSP2_SYNC_CPU_COUNT_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(~0UL, 2, 1, 1, 0, 1), OSP2_SYNC_CPU_COUNT_ERROR
    );

    failures += osp2_sync_expect(
        osp2_sync_validate(2, 0, 1, 1, 0, 1), OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 1, 1, 1, 0, 1), OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 3, 1, 1, 0, 1), OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, ~0UL, 1, 1, 0, 1),
        OSP2_SYNC_SHARED_TRACE_ERROR
    );

    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 0, 1, 0, 1), OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 2, 1, 0, 1), OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, ~0UL, 1, 0, 1),
        OSP2_SYNC_SHARED_TRACE_ERROR
    );

    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 0, 0, 1),
        OSP2_SYNC_NO_CONTENTION_ERROR
    );

    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, 1, 1), OSP2_SYNC_LOCK_HELD_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, 2, 1), OSP2_SYNC_LOCK_HELD_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, ~0UL, 1),
        OSP2_SYNC_LOCK_HELD_ERROR
    );

    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, 0, 0), OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, 0, 2), OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, 0, ~0UL),
        OSP2_SYNC_SHARED_TRACE_ERROR
    );

    failures += osp2_sync_expect(
        osp2_sync_validate(1, 0, 0, 0, 1, 0), OSP2_SYNC_CPU_COUNT_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 1, 0, 0, 1, 0),
        OSP2_SYNC_SHARED_TRACE_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 0, 1, 0),
        OSP2_SYNC_NO_CONTENTION_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_validate(2, 2, 1, 1, 1, 0),
        OSP2_SYNC_LOCK_HELD_ERROR
    );

    failures += osp2_sync_expect(osp2_sync_lock_order(0, 0), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(0, 1), OSP2_SYNC_OK);
    failures += osp2_sync_expect(osp2_sync_lock_order(0, 2), OSP2_SYNC_OK);
    failures += osp2_sync_expect(osp2_sync_lock_order(0, 3), OSP2_SYNC_OK);
    failures += osp2_sync_expect(osp2_sync_lock_order(0, 4), OSP2_SYNC_ORDER_ERROR);

    failures += osp2_sync_expect(osp2_sync_lock_order(1, 0), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(1, 1), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(1, 2), OSP2_SYNC_OK);
    failures += osp2_sync_expect(osp2_sync_lock_order(1, 3), OSP2_SYNC_OK);
    failures += osp2_sync_expect(osp2_sync_lock_order(1, 4), OSP2_SYNC_ORDER_ERROR);

    failures += osp2_sync_expect(osp2_sync_lock_order(2, 0), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(2, 1), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(2, 2), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(2, 3), OSP2_SYNC_OK);
    failures += osp2_sync_expect(osp2_sync_lock_order(2, 4), OSP2_SYNC_ORDER_ERROR);

    failures += osp2_sync_expect(osp2_sync_lock_order(3, 0), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(3, 1), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(3, 2), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(3, 3), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(3, 4), OSP2_SYNC_ORDER_ERROR);

    failures += osp2_sync_expect(osp2_sync_lock_order(4, 0), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(4, 1), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(4, 2), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(4, 3), OSP2_SYNC_ORDER_ERROR);
    failures += osp2_sync_expect(osp2_sync_lock_order(4, 4), OSP2_SYNC_ORDER_ERROR);

    failures += osp2_sync_expect(
        osp2_sync_lock_order(~0UL, 1), OSP2_SYNC_ORDER_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_lock_order(1, ~0UL), OSP2_SYNC_ORDER_ERROR
    );
    failures += osp2_sync_expect(
        osp2_sync_lock_order(~0UL, ~0UL), OSP2_SYNC_ORDER_ERROR
    );

    return failures;
}

#endif
