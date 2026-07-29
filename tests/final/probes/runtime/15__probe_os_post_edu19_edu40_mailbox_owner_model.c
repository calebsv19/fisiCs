/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-40-bounded-ap-mailbox-owner-generation, commit
 * 1efb1ac74e6b729982b8289abfc6879b13458b4b.
 *
 * Authoritative implementation: smp64.asm
 * SHA-256: 94b80d346fdd94cc58c34f26c78bee561ae772dbe6faa36801d9ceedb4b21cd5.
 * This C mirror probes the frozen singleton mailbox envelope only. It is not
 * OS source, publishes no AP work, and does not raise the active-runner limit.
 */
typedef unsigned char edu40_u8;
typedef unsigned int edu40_u32;
typedef unsigned long long edu40_u64;

enum {
    EDU40_MAILBOX_BYTES = 112,
    EDU40_MODE_SINGLE = 1,
    EDU40_MODE_PHASE_BATCH = 2,
    EDU40_PHASE_CONTEXT_COUNT = 2,
    EDU40_QUEUE_SLOT_COUNT = 8,
    EDU40_PHASE_RESULTS = 3,
    EDU40_LEGACY_RESULTS = 1
};

#define EDU40_OWNER_NONE (~0ULL)
#define EDU40_OWNER_LEGACY 2ULL
#define EDU40_LEGACY_REQUEST 0xED12000000000001ULL

static edu40_u32 edu40_read32(const edu40_u8 *bytes) {
    return (edu40_u32)bytes[0] | ((edu40_u32)bytes[1] << 8) |
           ((edu40_u32)bytes[2] << 16) | ((edu40_u32)bytes[3] << 24);
}

static edu40_u64 edu40_read64(const edu40_u8 *bytes) {
    return (edu40_u64)edu40_read32(bytes) |
           ((edu40_u64)edu40_read32(bytes + 4) << 32);
}

static int edu40_dispatch_owner_shape(
    const edu40_u8 *mailbox,
    edu40_u32 mode,
    edu40_u64 selected_context,
    edu40_u64 selected_slot,
    edu40_u64 selected_queue_generation,
    edu40_u64 selected_request) {
    edu40_u64 context = edu40_read64(mailbox + 32);
    edu40_u64 slot = edu40_read64(mailbox + 40);
    edu40_u64 queue_generation = edu40_read64(mailbox + 48);
    edu40_u64 request = edu40_read64(mailbox + 56);
    if (mode == EDU40_MODE_SINGLE) {
        return context == EDU40_OWNER_LEGACY &&
               slot == EDU40_OWNER_NONE &&
               queue_generation == 0 &&
               request == EDU40_LEGACY_REQUEST;
    }
    if (mode != EDU40_MODE_PHASE_BATCH ||
        context >= EDU40_PHASE_CONTEXT_COUNT ||
        slot >= EDU40_QUEUE_SLOT_COUNT ||
        queue_generation == 0 ||
        request == 0) return 0;
    return context == selected_context &&
           slot == selected_slot &&
           queue_generation == selected_queue_generation &&
           request == selected_request;
}

static int edu40_completion_tuples_match(const edu40_u8 *mailbox) {
    return edu40_read64(mailbox + 64) == edu40_read64(mailbox + 32) &&
           edu40_read64(mailbox + 72) == edu40_read64(mailbox + 40) &&
           edu40_read64(mailbox + 80) == edu40_read64(mailbox + 48) &&
           edu40_read64(mailbox + 88) == edu40_read64(mailbox + 56);
}

static int edu40_result_envelope_valid(
    const edu40_u8 *mailbox, edu40_u32 mode, edu40_u64 ap_error) {
    edu40_u64 expected;
    edu40_u64 results = edu40_read64(mailbox + 96);
    edu40_u64 generation = edu40_read64(mailbox + 16);
    edu40_u64 error_generation = edu40_read64(mailbox + 104);
    if (mode == EDU40_MODE_SINGLE) expected = EDU40_LEGACY_RESULTS;
    else if (mode == EDU40_MODE_PHASE_BATCH) expected = EDU40_PHASE_RESULTS;
    else return 0;
    if (ap_error == 0) {
        return results == expected && error_generation == 0;
    }
    return results <= expected && error_generation == generation;
}

int edu40_mailbox_reset_valid(const edu40_u8 *mailbox) {
    if (mailbox == (const edu40_u8 *)0) return 0;
    return edu40_read64(mailbox) == 0 &&
           edu40_read64(mailbox + 8) == 0 &&
           edu40_read64(mailbox + 16) == 0 &&
           edu40_read64(mailbox + 24) == 0 &&
           edu40_read64(mailbox + 32) == EDU40_OWNER_NONE &&
           edu40_read64(mailbox + 40) == EDU40_OWNER_NONE &&
           edu40_read64(mailbox + 48) == 0 &&
           edu40_read64(mailbox + 56) == 0 &&
           edu40_read64(mailbox + 64) == EDU40_OWNER_NONE &&
           edu40_read64(mailbox + 72) == EDU40_OWNER_NONE &&
           edu40_read64(mailbox + 80) == 0 &&
           edu40_read64(mailbox + 88) == 0 &&
           edu40_read64(mailbox + 96) == 0 &&
           edu40_read64(mailbox + 104) == 0;
}

int edu40_mailbox_idle_can_begin(const edu40_u8 *mailbox) {
    edu40_u64 generation;
    if (mailbox == (const edu40_u8 *)0 ||
        edu40_read64(mailbox) != 0 ||
        edu40_read64(mailbox + 8) != 0) return 0;
    generation = edu40_read64(mailbox + 16);
    return generation == edu40_read64(mailbox + 24) &&
           generation != ~0ULL;
}

edu40_u64 edu40_mailbox_next_generation(const edu40_u8 *mailbox) {
    if (!edu40_mailbox_idle_can_begin(mailbox)) return 0;
    return edu40_read64(mailbox + 16) + 1ULL;
}

int edu40_mailbox_dispatch_valid(
    const edu40_u8 *mailbox,
    edu40_u64 work_generation,
    edu40_u32 mode,
    edu40_u64 selected_context,
    edu40_u64 selected_slot,
    edu40_u64 selected_queue_generation,
    edu40_u64 selected_request) {
    edu40_u64 generation;
    if (mailbox == (const edu40_u8 *)0 ||
        edu40_read64(mailbox) != 1 ||
        edu40_read64(mailbox + 8) != 1) return 0;
    generation = edu40_read64(mailbox + 16);
    if (generation == 0 || work_generation != generation ||
        edu40_read64(mailbox + 24) + 1ULL != generation ||
        edu40_read64(mailbox + 64) != EDU40_OWNER_NONE ||
        edu40_read64(mailbox + 72) != EDU40_OWNER_NONE ||
        edu40_read64(mailbox + 80) != 0 ||
        edu40_read64(mailbox + 88) != 0 ||
        edu40_read64(mailbox + 96) != 0 ||
        edu40_read64(mailbox + 104) != 0) return 0;
    return edu40_dispatch_owner_shape(
        mailbox, mode, selected_context, selected_slot,
        selected_queue_generation, selected_request);
}

int edu40_mailbox_ap_accepts(
    const edu40_u8 *mailbox,
    edu40_u64 seen_generation,
    edu40_u64 work_generation,
    edu40_u32 mode) {
    edu40_u64 generation;
    if (mailbox == (const edu40_u8 *)0 ||
        seen_generation == ~0ULL ||
        edu40_read64(mailbox) != 1 ||
        edu40_read64(mailbox + 8) != 1) return 0;
    generation = edu40_read64(mailbox + 16);
    if (generation == 0 ||
        work_generation != generation ||
        seen_generation + 1ULL != generation) return 0;
    if (mode == EDU40_MODE_SINGLE) {
        return edu40_dispatch_owner_shape(
            mailbox, mode, 0, 0, 0, 0);
    }
    return mode == EDU40_MODE_PHASE_BATCH &&
           edu40_read64(mailbox + 32) < EDU40_PHASE_CONTEXT_COUNT &&
           edu40_read64(mailbox + 40) < EDU40_QUEUE_SLOT_COUNT &&
           edu40_read64(mailbox + 48) != 0 &&
           edu40_read64(mailbox + 56) != 0;
}

int edu40_mailbox_completion_publishable(
    const edu40_u8 *mailbox, edu40_u64 observed_generation) {
    if (mailbox == (const edu40_u8 *)0 ||
        edu40_read64(mailbox) != 1 ||
        edu40_read64(mailbox + 8) != 1 ||
        edu40_read64(mailbox + 16) != observed_generation) return 0;
    return edu40_read64(mailbox + 24) < observed_generation;
}

int edu40_mailbox_completion_valid(
    const edu40_u8 *mailbox,
    edu40_u64 work_generation,
    edu40_u32 mode,
    edu40_u64 ap_error,
    edu40_u64 selected_context,
    edu40_u64 selected_slot,
    edu40_u64 selected_queue_generation,
    edu40_u64 selected_request) {
    edu40_u64 generation;
    if (mailbox == (const edu40_u8 *)0 ||
        edu40_read64(mailbox) != 1 ||
        edu40_read64(mailbox + 8) != 1) return 0;
    generation = edu40_read64(mailbox + 16);
    if (generation == 0 ||
        work_generation != generation ||
        edu40_read64(mailbox + 24) != generation ||
        !edu40_completion_tuples_match(mailbox) ||
        !edu40_dispatch_owner_shape(
            mailbox, mode, selected_context, selected_slot,
            selected_queue_generation, selected_request)) return 0;
    return edu40_result_envelope_valid(mailbox, mode, ap_error);
}

int edu40_mailbox_retired_valid(
    const edu40_u8 *mailbox,
    edu40_u32 mode,
    edu40_u64 ap_error,
    edu40_u64 selected_context,
    edu40_u64 selected_slot,
    edu40_u64 selected_queue_generation,
    edu40_u64 selected_request) {
    edu40_u64 generation;
    if (mailbox == (const edu40_u8 *)0 ||
        edu40_read64(mailbox) != 0 ||
        edu40_read64(mailbox + 8) != 0) return 0;
    generation = edu40_read64(mailbox + 16);
    if (generation == 0 ||
        edu40_read64(mailbox + 24) != generation ||
        !edu40_completion_tuples_match(mailbox) ||
        !edu40_dispatch_owner_shape(
            mailbox, mode, selected_context, selected_slot,
            selected_queue_generation, selected_request)) return 0;
    return edu40_result_envelope_valid(mailbox, mode, ap_error);
}

int edu40_mailbox_unchanged(
    const edu40_u8 *before, const edu40_u8 *after) {
    edu40_u32 index;
    if (before == (const edu40_u8 *)0 ||
        after == (const edu40_u8 *)0) return 0;
    for (index = 0; index < EDU40_MAILBOX_BYTES; index = index + 1) {
        if (before[index] != after[index]) return 0;
    }
    return 1;
}
