// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_SCHEDULER_TRANSITION_VECTORS_H
#define OSP2_SCHEDULER_TRANSITION_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_SCHED_RUNNABLE 1UL
#define OSP2_SCHED_SLEEPING 2UL
#define OSP2_SCHED_TERMINATED 3UL
#define OSP2_SCHED_FAULTED 4UL
#define OSP2_SCHED_YIELD 1UL
#define OSP2_SCHED_SLEEP 2UL
#define OSP2_SCHED_TIMER 3UL
#define OSP2_SCHED_EXIT 4UL
#define OSP2_SCHED_FAULT 5UL
#define OSP2_SCHED_VECTOR_COUNT 60UL
#define OSP2_SCHED_CORPUS_ID "sched60-v1"

void osp2_scheduler_init(void);
osp2_u64 osp2_scheduler_choose(
    osp2_u64 current,
    osp2_u64 reason,
    osp2_u64 now,
    osp2_u64 sleep_ticks
);
void osp2_scheduler_reactivate(osp2_u64 task);
osp2_u64 osp2_scheduler_state(osp2_u64 task);
osp2_u64 osp2_scheduler_switch_count(void);
osp2_u64 osp2_scheduler_yield_count(void);
osp2_u64 osp2_scheduler_sleep_count(void);
osp2_u64 osp2_scheduler_preemption_count(void);

static osp2_u64 osp2_scheduler_expect(
    osp2_u64 actual,
    osp2_u64 expected
) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_scheduler_run_vectors(void) {
    osp2_u64 failures = 0;

    osp2_scheduler_init();
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(0), OSP2_SCHED_RUNNABLE
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_RUNNABLE
    );
    failures += osp2_scheduler_expect(osp2_scheduler_state(2), 0);
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 0);
    failures += osp2_scheduler_expect(osp2_scheduler_yield_count(), 0);
    failures += osp2_scheduler_expect(osp2_scheduler_sleep_count(), 0);
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 0);

    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(0, OSP2_SCHED_YIELD, 0, 0), 1
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(0), OSP2_SCHED_RUNNABLE
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_RUNNABLE
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);
    failures += osp2_scheduler_expect(osp2_scheduler_yield_count(), 1);
    failures += osp2_scheduler_expect(osp2_scheduler_sleep_count(), 0);
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 0);

    osp2_scheduler_init();
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(0, OSP2_SCHED_TIMER, 5, 0), 1
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 1);
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(1, OSP2_SCHED_TIMER, 6, 0), 0
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 2);
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 2);

    osp2_scheduler_init();
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(0, OSP2_SCHED_SLEEP, 10, 3), 1
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(0), OSP2_SCHED_SLEEPING
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_RUNNABLE
    );
    failures += osp2_scheduler_expect(osp2_scheduler_sleep_count(), 1);
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(1, OSP2_SCHED_TIMER, 12, 0), 1
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(0), OSP2_SCHED_SLEEPING
    );
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 1);
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(1, OSP2_SCHED_TIMER, 13, 0), 0
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(0), OSP2_SCHED_RUNNABLE
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 2);
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 2);

    osp2_scheduler_init();
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(1, OSP2_SCHED_SLEEP, 20, 2), 0
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_SLEEPING
    );
    failures += osp2_scheduler_expect(osp2_scheduler_sleep_count(), 1);
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);

    osp2_scheduler_init();
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(1, OSP2_SCHED_EXIT, 30, 0), 0
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_TERMINATED
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(0, OSP2_SCHED_TIMER, 31, 0), 0
    );
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 1);
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);
    osp2_scheduler_reactivate(1);
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_RUNNABLE
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(0, OSP2_SCHED_YIELD, 32, 0), 1
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 2);
    failures += osp2_scheduler_expect(osp2_scheduler_yield_count(), 1);

    osp2_scheduler_init();
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(0, OSP2_SCHED_FAULT, 40, 0), 1
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(0), OSP2_SCHED_FAULTED
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(1, OSP2_SCHED_EXIT, 41, 0), 0
    );
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_TERMINATED
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 1);

    osp2_scheduler_reactivate(0);
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(0), OSP2_SCHED_RUNNABLE
    );
    osp2_scheduler_reactivate(1);
    failures += osp2_scheduler_expect(
        osp2_scheduler_state(1), OSP2_SCHED_RUNNABLE
    );
    osp2_scheduler_reactivate(9);
    failures += osp2_scheduler_expect(osp2_scheduler_state(9), 0);
    failures += osp2_scheduler_expect(
        osp2_scheduler_choose(0, 99, 50, 99), 1
    );
    failures += osp2_scheduler_expect(osp2_scheduler_switch_count(), 2);
    failures += osp2_scheduler_expect(osp2_scheduler_yield_count(), 0);
    failures += osp2_scheduler_expect(osp2_scheduler_sleep_count(), 0);
    failures += osp2_scheduler_expect(osp2_scheduler_preemption_count(), 0);
    return failures;
}

#endif
