/*
 * Compiler-side cooperative-runner mirror derived from immutable os-dev tags:
 *   EDU-29 asynchronous activation
 *   commit 2b8189a24e1c2b93fb709603698b2d86987e66f5
 *   EDU-30 running cancellation and logical budgets
 *   commit fac8fd447acd789397253308ab27375305af754f
 *
 * EDU-29 changes authoritative assembly plus Wire-v5 admission. EDU-30's
 * exact queue_kernel.c SHA-256 is
 * 48b0ea8d19f48e9fb6d105d64049b7b319f63173098bb067b067c366c7a9cbcb.
 *
 * This model covers the bounded one-runner activation/stop decisions only.
 * Persistence, resource release, and phase execution remain OS-owned.
 */
typedef unsigned int edu30_u32;
typedef unsigned long long edu30_u64;

enum {
    EDU30_STATE_EMPTY = 0,
    EDU30_STATE_PENDING = 1,
    EDU30_STATE_RUNNING = 2,
    EDU30_STATE_COMPLETE = 3,
    EDU30_STATE_FAILED = 4,
    EDU30_STATE_CANCELLED = 5,
    EDU29_SELECT_NONE = 8,
    EDU29_SELECT_BUSY = 9,
    EDU29_SELECT_CORRUPT = 10,
    EDU30_STOP_CANCEL = 1,
    EDU30_STOP_BUDGET = 2,
    EDU30_STOP_WORK = 3,
    EDU30_STOP_CORRUPT = 4,
    EDU30_FLAG_CANCEL_PENDING = 1,
    EDU30_FLAG_CANCEL_RUNNING = 2,
    EDU30_REASON_CANCELLED = 3,
    EDU30_REASON_BUDGET = 8
};

edu30_u64 edu29_activation_select(
    const edu30_u32 *states,
    const edu30_u32 *entry_valid,
    edu30_u32 slot_count,
    edu30_u32 runner_active) {
    edu30_u32 slot;
    if (states == (const edu30_u32 *)0 ||
        entry_valid == (const edu30_u32 *)0 ||
        slot_count > 8 || runner_active > 1) return EDU29_SELECT_CORRUPT;
    if (runner_active != 0) return EDU29_SELECT_BUSY;
    for (slot = 0; slot < slot_count; slot = slot + 1) {
        if (entry_valid[slot] > 1) return EDU29_SELECT_CORRUPT;
        if (states[slot] == EDU30_STATE_EMPTY) {
            if (entry_valid[slot] != 0) return EDU29_SELECT_CORRUPT;
        } else {
            if (entry_valid[slot] == 0 ||
                states[slot] > EDU30_STATE_CANCELLED) {
                return EDU29_SELECT_CORRUPT;
            }
            if (states[slot] == EDU30_STATE_RUNNING) {
                return EDU29_SELECT_CORRUPT;
            }
            if (states[slot] == EDU30_STATE_PENDING) return slot;
        }
    }
    return EDU29_SELECT_NONE;
}

int edu29_runner_identity_valid(
    edu30_u32 runner_active,
    edu30_u64 runner_slot,
    edu30_u64 runner_generation,
    edu30_u64 runner_request,
    edu30_u32 entry_state,
    edu30_u64 entry_slot,
    edu30_u64 entry_generation,
    edu30_u64 entry_request,
    edu30_u32 phase) {
    if (runner_active != 1 ||
        runner_slot >= 8 ||
        runner_generation == 0 ||
        runner_request == 0 ||
        entry_state != EDU30_STATE_RUNNING ||
        phase > 4) return 0;
    return runner_slot == entry_slot &&
           runner_generation == entry_generation &&
           runner_request == entry_request;
}

int edu30_work_shape_valid(
    edu30_u32 requested_work,
    edu30_u32 effective_work,
    edu30_u32 consumed_work,
    edu30_u32 phase) {
    return requested_work >= 1 && requested_work <= 4 &&
           effective_work == requested_work &&
           consumed_work == phase &&
           phase <= 4;
}

edu30_u64 edu30_budget_value(
    edu30_u32 requested_work,
    edu30_u32 effective_work,
    edu30_u32 consumed_work) {
    if (requested_work > 0xffffU ||
        effective_work > 0xffffU) return ~0ULL;
    return (edu30_u64)requested_work |
           ((edu30_u64)effective_work << 16) |
           ((edu30_u64)consumed_work << 32);
}

edu30_u64 edu30_stop_action(
    edu30_u32 cancel_running,
    edu30_u32 requested_work,
    edu30_u32 effective_work,
    edu30_u32 consumed_work,
    edu30_u32 phase) {
    if (cancel_running > 1 ||
        !edu30_work_shape_valid(
            requested_work, effective_work, consumed_work, phase)) {
        return EDU30_STOP_CORRUPT;
    }
    if (cancel_running != 0) return EDU30_STOP_CANCEL;
    if (phase < 4 && consumed_work >= effective_work) {
        return EDU30_STOP_BUDGET;
    }
    return EDU30_STOP_WORK;
}

int edu30_cancel_terminal_valid(
    edu30_u32 was_running,
    edu30_u32 flags,
    edu30_u32 requested_work,
    edu30_u32 effective_work,
    edu30_u32 consumed_work,
    edu30_u32 phase,
    edu30_u32 reason,
    edu30_u64 terminal_value,
    edu30_u32 cancellation_event_seen) {
    edu30_u64 expected;
    if (was_running > 1 || cancellation_event_seen > 1 ||
        reason != EDU30_REASON_CANCELLED ||
        requested_work < 1 || requested_work > 4) return 0;
    if (was_running == 0) {
        expected = edu30_budget_value(requested_work, 0, 0);
        return flags == EDU30_FLAG_CANCEL_PENDING &&
               effective_work == 0 && consumed_work == 0 && phase == 0 &&
               cancellation_event_seen != 0 &&
               terminal_value == expected;
    }
    if (!edu30_work_shape_valid(
            requested_work, effective_work, consumed_work, phase)) return 0;
    expected = edu30_budget_value(
        requested_work, effective_work, consumed_work);
    return flags == EDU30_FLAG_CANCEL_RUNNING &&
           cancellation_event_seen != 0 &&
           terminal_value == expected;
}

int edu30_budget_terminal_valid(
    edu30_u32 requested_work,
    edu30_u32 effective_work,
    edu30_u32 consumed_work,
    edu30_u32 phase,
    edu30_u32 reason,
    edu30_u64 terminal_value,
    edu30_u32 running_published) {
    if (running_published != 1 ||
        reason != EDU30_REASON_BUDGET ||
        !edu30_work_shape_valid(
            requested_work, effective_work, consumed_work, phase) ||
        phase >= 4 || consumed_work != effective_work) return 0;
    return terminal_value ==
        edu30_budget_value(requested_work, effective_work, consumed_work);
}
