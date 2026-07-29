/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-41-two-active-deterministic-cooperative-runners, commit
 * 695ec663cb419c1b6604c9fe8777a48ceb81d5cb.
 *
 * Authoritative implementation: queue64.asm and smp64.asm
 * SHA-256:
 *   queue64.asm 1b3e3b1b59f0110d8aae3f7be7d393c45c8514d0dc16804893b3b8175de85ec8
 *   smp64.asm   c044735c0a5bc462b7aa6ea7dc2f914c924ec35a29261978496785a38bb78f24
 *
 * This hardware-blind C mirror probes only the frozen two-runner selection
 * and ownership envelope. It is not OS source, does not publish queue work,
 * and does not model a general symmetric scheduler.
 */
typedef unsigned char edu41_u8;
typedef unsigned int edu41_u32;
typedef unsigned long long edu41_u64;

enum {
    EDU41_CONTEXT_COUNT = 2,
    EDU41_CONTEXT_BYTES = 160,
    EDU41_QUEUE_SLOTS = 8,
    EDU41_MAILBOX_BYTES = 112,
    EDU41_STATE_EMPTY = 0,
    EDU41_STATE_PENDING = 1,
    EDU41_STATE_RUNNING = 2,
    EDU41_STATE_COMPLETE = 3,
    EDU41_STATE_FAILED = 4,
    EDU41_STATE_CANCELLED = 5,
    EDU41_SELECT_NONE = 8,
    EDU41_SELECT_BUSY = 9,
    EDU41_SELECT_CORRUPT = 10,
    EDU41_CONTEXT_NONE = 2,
    EDU41_ACTION_CANCEL = 1,
    EDU41_ACTION_TIMEOUT = 2,
    EDU41_ACTION_BUDGET = 3,
    EDU41_ACTION_WORK = 4,
    EDU41_ACTION_CORRUPT = 5
};

static edu41_u32 edu41_read32(const edu41_u8 *bytes) {
    return (edu41_u32)bytes[0] | ((edu41_u32)bytes[1] << 8) |
           ((edu41_u32)bytes[2] << 16) | ((edu41_u32)bytes[3] << 24);
}

static edu41_u64 edu41_read64(const edu41_u8 *bytes) {
    return (edu41_u64)edu41_read32(bytes) |
           ((edu41_u64)edu41_read32(bytes + 4) << 32);
}

static const edu41_u8 *edu41_context(
    const edu41_u8 *contexts, edu41_u32 context_id) {
    return contexts + context_id * EDU41_CONTEXT_BYTES;
}

edu41_u64 edu41_active_count(const edu41_u8 *contexts) {
    edu41_u32 context_id;
    edu41_u64 count = 0;
    if (contexts == (const edu41_u8 *)0) return ~0ULL;
    for (context_id = 0; context_id < EDU41_CONTEXT_COUNT;
         context_id = context_id + 1) {
        const edu41_u8 *record = edu41_context(contexts, context_id);
        edu41_u64 active = edu41_read64(record);
        if (edu41_read64(record + 8) != context_id || active > 1) {
            return ~0ULL;
        }
        count = count + active;
    }
    return count;
}

int edu41_running_owner_valid(
    const edu41_u8 *contexts,
    edu41_u64 slot,
    edu41_u64 entry_address,
    edu41_u64 generation,
    edu41_u64 request) {
    const edu41_u8 *record;
    edu41_u32 context_id;
    if (contexts == (const edu41_u8 *)0 ||
        slot >= EDU41_QUEUE_SLOTS ||
        entry_address == 0 ||
        generation == 0 ||
        request == 0 ||
        edu41_active_count(contexts) == ~0ULL) return 0;
    context_id = (edu41_u32)(slot & 1ULL);
    record = edu41_context(contexts, context_id);
    return edu41_read64(record) == 1 &&
           edu41_read64(record + 8) == context_id &&
           edu41_read64(record + 16) == slot &&
           edu41_read64(record + 24) == entry_address &&
           edu41_read64(record + 32) == generation &&
           edu41_read64(record + 40) == request;
}

edu41_u64 edu41_activation_select(
    const edu41_u8 *contexts,
    const edu41_u32 *states,
    const edu41_u64 *entry_addresses,
    const edu41_u64 *generations,
    const edu41_u64 *requests,
    edu41_u32 slot_count) {
    edu41_u64 active_count;
    edu41_u32 slot;
    int saw_ineligible = 0;
    if (contexts == (const edu41_u8 *)0 ||
        states == (const edu41_u32 *)0 ||
        entry_addresses == (const edu41_u64 *)0 ||
        generations == (const edu41_u64 *)0 ||
        requests == (const edu41_u64 *)0 ||
        slot_count > EDU41_QUEUE_SLOTS) return EDU41_SELECT_CORRUPT;
    active_count = edu41_active_count(contexts);
    if (active_count == ~0ULL) return EDU41_SELECT_CORRUPT;
    if (active_count >= EDU41_CONTEXT_COUNT) return EDU41_SELECT_BUSY;
    for (slot = 0; slot < slot_count; slot = slot + 1) {
        edu41_u32 state = states[slot];
        if (state == EDU41_STATE_EMPTY) {
            if (entry_addresses[slot] != 0 ||
                generations[slot] != 0 ||
                requests[slot] != 0) return EDU41_SELECT_CORRUPT;
        } else if (state == EDU41_STATE_PENDING) {
            const edu41_u8 *record =
                edu41_context(contexts, slot & 1U);
            if (entry_addresses[slot] == 0 ||
                generations[slot] == 0 ||
                requests[slot] == 0) return EDU41_SELECT_CORRUPT;
            if (edu41_read64(record) == 0) return slot;
            saw_ineligible = 1;
        } else if (state == EDU41_STATE_RUNNING) {
            if (!edu41_running_owner_valid(
                    contexts, slot, entry_addresses[slot],
                    generations[slot], requests[slot])) {
                return EDU41_SELECT_CORRUPT;
            }
        } else if (state == EDU41_STATE_COMPLETE ||
                   state == EDU41_STATE_FAILED ||
                   state == EDU41_STATE_CANCELLED) {
            if (entry_addresses[slot] == 0 ||
                generations[slot] == 0 ||
                requests[slot] == 0) return EDU41_SELECT_CORRUPT;
        } else {
            return EDU41_SELECT_CORRUPT;
        }
    }
    return saw_ineligible ? EDU41_SELECT_BUSY : EDU41_SELECT_NONE;
}

edu41_u64 edu41_mailbox_phase_owner(const edu41_u8 *mailbox) {
    edu41_u64 owner_valid;
    edu41_u64 inflight;
    edu41_u64 context_id;
    if (mailbox == (const edu41_u8 *)0) return ~0ULL;
    owner_valid = edu41_read64(mailbox);
    inflight = edu41_read64(mailbox + 8);
    if (owner_valid != inflight || owner_valid > 1) return ~0ULL;
    if (owner_valid == 0) return EDU41_CONTEXT_NONE;
    context_id = edu41_read64(mailbox + 32);
    if (context_id >= EDU41_CONTEXT_COUNT) return ~0ULL;
    return context_id;
}

edu41_u64 edu41_choose_context(
    const edu41_u8 *contexts,
    edu41_u64 next_context,
    const edu41_u8 *mailbox) {
    edu41_u64 active_count = edu41_active_count(contexts);
    edu41_u64 mailbox_owner;
    const edu41_u8 *record;
    if (active_count == ~0ULL) return ~0ULL;
    if (active_count == 0) return EDU41_CONTEXT_NONE;
    mailbox_owner = edu41_mailbox_phase_owner(mailbox);
    if (mailbox_owner == ~0ULL) return ~0ULL;
    if (mailbox_owner != EDU41_CONTEXT_NONE) {
        record = edu41_context(contexts, (edu41_u32)mailbox_owner);
        return edu41_read64(record) == 1 ? mailbox_owner : ~0ULL;
    }
    if (next_context >= EDU41_CONTEXT_COUNT) return ~0ULL;
    record = edu41_context(contexts, (edu41_u32)next_context);
    if (edu41_read64(record) == 1) return next_context;
    next_context = next_context ^ 1ULL;
    record = edu41_context(contexts, (edu41_u32)next_context);
    return edu41_read64(record) == 1 ? next_context : ~0ULL;
}

edu41_u64 edu41_next_turn(edu41_u64 selected_context) {
    if (selected_context >= EDU41_CONTEXT_COUNT) return ~0ULL;
    return selected_context ^ 1ULL;
}

edu41_u64 edu41_release_phase_owner(
    edu41_u64 selected_phase_context, edu41_u64 retiring_context) {
    if (retiring_context >= EDU41_CONTEXT_COUNT ||
        (selected_phase_context != EDU41_CONTEXT_NONE &&
         selected_phase_context >= EDU41_CONTEXT_COUNT)) return ~0ULL;
    if (selected_phase_context == retiring_context) return EDU41_CONTEXT_NONE;
    return selected_phase_context;
}

edu41_u64 edu41_policy_action(
    edu41_u32 cancel_running,
    edu41_u32 time_valid,
    edu41_u64 now_ns,
    edu41_u64 deadline_ns,
    edu41_u64 consumed_work,
    edu41_u64 effective_work,
    edu41_u64 phase) {
    if (cancel_running > 1 || time_valid > 1 ||
        phase > 4 || consumed_work != phase ||
        effective_work > 4) return EDU41_ACTION_CORRUPT;
    if (cancel_running != 0) return EDU41_ACTION_CANCEL;
    if (time_valid == 0) return EDU41_ACTION_CORRUPT;
    if (now_ns >= deadline_ns) return EDU41_ACTION_TIMEOUT;
    if (phase < 4 && consumed_work >= effective_work) {
        return EDU41_ACTION_BUDGET;
    }
    return EDU41_ACTION_WORK;
}

int edu41_retirement_preserves_peer(
    const edu41_u8 *before,
    const edu41_u8 *after,
    edu41_u32 retiring_context) {
    edu41_u32 peer;
    edu41_u32 index;
    const edu41_u8 *before_peer;
    const edu41_u8 *after_peer;
    const edu41_u8 *after_owner;
    if (before == (const edu41_u8 *)0 ||
        after == (const edu41_u8 *)0 ||
        retiring_context >= EDU41_CONTEXT_COUNT) return 0;
    peer = retiring_context ^ 1U;
    before_peer = edu41_context(before, peer);
    after_peer = edu41_context(after, peer);
    after_owner = edu41_context(after, retiring_context);
    if (edu41_read64(after_owner) != 0 ||
        edu41_read64(after_owner + 8) != retiring_context) return 0;
    for (index = 0; index < EDU41_CONTEXT_BYTES; index = index + 1) {
        if (before_peer[index] != after_peer[index]) return 0;
    }
    return 1;
}
