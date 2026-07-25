// SPDX-License-Identifier: Apache-2.0

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

static volatile osp2_u64 osp2_sched_task0_state;
static volatile osp2_u64 osp2_sched_task1_state;
static volatile osp2_u64 osp2_sched_task0_wake_tick;
static volatile osp2_u64 osp2_sched_task1_wake_tick;
static volatile osp2_u64 osp2_sched_switches;
static volatile osp2_u64 osp2_sched_yields;
static volatile osp2_u64 osp2_sched_sleeps;
static volatile osp2_u64 osp2_sched_preemptions;

void osp2_scheduler_init(void) {
    osp2_sched_task0_state = OSP2_SCHED_RUNNABLE;
    osp2_sched_task1_state = OSP2_SCHED_RUNNABLE;
    osp2_sched_task0_wake_tick = 0;
    osp2_sched_task1_wake_tick = 0;
    osp2_sched_switches = 0;
    osp2_sched_yields = 0;
    osp2_sched_sleeps = 0;
    osp2_sched_preemptions = 0;
}

osp2_u64 osp2_scheduler_choose(
    osp2_u64 current,
    osp2_u64 reason,
    osp2_u64 now,
    osp2_u64 sleep_ticks
) {
    osp2_u64 other = current ^ 1UL;

    if (osp2_sched_task0_state == OSP2_SCHED_SLEEPING &&
        now >= osp2_sched_task0_wake_tick) {
        osp2_sched_task0_state = OSP2_SCHED_RUNNABLE;
    }
    if (osp2_sched_task1_state == OSP2_SCHED_SLEEPING &&
        now >= osp2_sched_task1_wake_tick) {
        osp2_sched_task1_state = OSP2_SCHED_RUNNABLE;
    }

    if (reason == OSP2_SCHED_SLEEP) {
        if (current == 0) {
            osp2_sched_task0_state = OSP2_SCHED_SLEEPING;
            osp2_sched_task0_wake_tick = now + sleep_ticks;
        } else {
            osp2_sched_task1_state = OSP2_SCHED_SLEEPING;
            osp2_sched_task1_wake_tick = now + sleep_ticks;
        }
        osp2_sched_sleeps = osp2_sched_sleeps + 1;
    } else if (reason == OSP2_SCHED_YIELD) {
        osp2_sched_yields = osp2_sched_yields + 1;
    } else if (reason == OSP2_SCHED_TIMER) {
        osp2_sched_preemptions = osp2_sched_preemptions + 1;
    } else if (reason == OSP2_SCHED_EXIT) {
        if (current == 0) {
            osp2_sched_task0_state = OSP2_SCHED_TERMINATED;
        } else {
            osp2_sched_task1_state = OSP2_SCHED_TERMINATED;
        }
    } else if (reason == OSP2_SCHED_FAULT) {
        if (current == 0) {
            osp2_sched_task0_state = OSP2_SCHED_FAULTED;
        } else {
            osp2_sched_task1_state = OSP2_SCHED_FAULTED;
        }
    }

    if (other == 0 && osp2_sched_task0_state == OSP2_SCHED_RUNNABLE) {
        osp2_sched_switches = osp2_sched_switches + 1;
        return 0;
    }
    if (other == 1 && osp2_sched_task1_state == OSP2_SCHED_RUNNABLE) {
        osp2_sched_switches = osp2_sched_switches + 1;
        return 1;
    }
    if (current == 0 && osp2_sched_task0_state == OSP2_SCHED_RUNNABLE) {
        return 0;
    }
    if (current == 1 && osp2_sched_task1_state == OSP2_SCHED_RUNNABLE) {
        return 1;
    }
    return 0;
}

void osp2_scheduler_reactivate(osp2_u64 task) {
    if (task == 0) {
        osp2_sched_task0_state = OSP2_SCHED_RUNNABLE;
        osp2_sched_task0_wake_tick = 0;
    } else if (task == 1) {
        osp2_sched_task1_state = OSP2_SCHED_RUNNABLE;
        osp2_sched_task1_wake_tick = 0;
    }
}

osp2_u64 osp2_scheduler_state(osp2_u64 task) {
    if (task == 0) {
        return osp2_sched_task0_state;
    }
    if (task == 1) {
        return osp2_sched_task1_state;
    }
    return 0;
}

osp2_u64 osp2_scheduler_switch_count(void) {
    return osp2_sched_switches;
}

osp2_u64 osp2_scheduler_yield_count(void) {
    return osp2_sched_yields;
}

osp2_u64 osp2_scheduler_sleep_count(void) {
    return osp2_sched_sleeps;
}

osp2_u64 osp2_scheduler_preemption_count(void) {
    return osp2_sched_preemptions;
}
