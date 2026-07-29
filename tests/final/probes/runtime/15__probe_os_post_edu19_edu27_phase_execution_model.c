/*
 * Compiler-side phase-evidence mirror derived from immutable os-dev tag
 * edu-27-bounded-phase-aware-execution, commit
 * 9c9e2b04720a3de91e52656c86a3dc5520a7eb86.
 *
 * Exact queue_kernel.c SHA-256:
 * 6fad79b28e9bdb681eaa43cbca571e13066c8cb5f2f9c21ce3605fc140108681
 *
 * The model covers Trace-v2 phase ordering and width-dependent evidence.
 * Durable publication, recovery writes, and BSP/AP work remain OS-owned.
 */
typedef unsigned char edu27_u8;
typedef unsigned int edu27_u32;
typedef unsigned long long edu27_u64;

enum {
    EDU27_STATE_RUNNING = 2,
    EDU27_EVENT_SETUP = 15,
    EDU27_EVENT_COMPUTE = 16,
    EDU27_EVENT_BARRIER = 17,
    EDU27_EVENT_REDUCE = 18
};

static edu27_u32 edu27_read16(const edu27_u8 *p) {
    return (edu27_u32)p[0] | ((edu27_u32)p[1] << 8);
}

static edu27_u32 edu27_read32(const edu27_u8 *p) {
    return (edu27_u32)p[0] | ((edu27_u32)p[1] << 8) |
           ((edu27_u32)p[2] << 16) | ((edu27_u32)p[3] << 24);
}

static edu27_u64 edu27_read64(const edu27_u8 *p) {
    return (edu27_u64)edu27_read32(p) |
           ((edu27_u64)edu27_read32(p + 4) << 32);
}

edu27_u64 edu27_expected_phase_value(
    edu27_u32 effective_workers, edu27_u32 phase_stage) {
    if (effective_workers < 1 || effective_workers > 2 ||
        phase_stage < 1 || phase_stage > 4) return ~0ULL;
    if (phase_stage == 1) {
        return (edu27_u64)effective_workers | (3ULL << 32);
    }
    if (phase_stage == 2) {
        return (3ULL << 48) |
            (effective_workers == 1 ?
             6ULL :
             (3ULL | (1ULL << 16) | (3ULL << 32)));
    }
    if (phase_stage == 3) {
        return (edu27_u64)effective_workers |
               ((edu27_u64)effective_workers << 16) |
               ((edu27_u64)(effective_workers == 2 ? 1 : 0) << 32) |
               (3ULL << 48);
    }
    return 0x6EC4E5DB9E1056CFULL;
}

edu27_u32 edu27_event_advance(
    edu27_u32 current_stage, const edu27_u8 *event) {
    edu27_u32 kind;
    edu27_u32 next_stage;
    if (event == (const edu27_u8 *)0 || current_stage > 4) {
        return 0xffffffffU;
    }
    kind = edu27_read16(event + 4);
    if (kind < EDU27_EVENT_SETUP || kind > EDU27_EVENT_REDUCE) {
        return 0xffffffffU;
    }
    next_stage = kind - EDU27_EVENT_SETUP + 1U;
    if (next_stage != current_stage + 1U ||
        edu27_read32(event + 16) != EDU27_STATE_RUNNING ||
        edu27_read32(event + 20) != next_stage) {
        return 0xffffffffU;
    }
    return next_stage;
}

int edu27_phase_prefix_valid(
    edu27_u32 effective_workers,
    edu27_u32 phase_stage,
    edu27_u64 result,
    edu27_u64 setup_value,
    edu27_u64 compute_value,
    edu27_u64 barrier_value,
    edu27_u64 reduce_value) {
    edu27_u32 stage;
    edu27_u64 values[4];
    if (effective_workers < 1 || effective_workers > 2 ||
        phase_stage > 4) return 0;
    values[0] = setup_value;
    values[1] = compute_value;
    values[2] = barrier_value;
    values[3] = reduce_value;
    for (stage = 1; stage <= 4; stage = stage + 1) {
        if (stage <= phase_stage) {
            if (values[stage - 1] !=
                edu27_expected_phase_value(effective_workers, stage)) return 0;
        } else if (values[stage - 1] != 0) {
            return 0;
        }
    }
    if (phase_stage < 4) return result == 0;
    return result == 0x6EC4E5DB9E1056CFULL;
}

int edu27_trace_prefix_valid(
    edu27_u32 effective_workers,
    const edu27_u8 *events,
    edu27_u32 event_count,
    edu27_u64 result) {
    edu27_u32 stage = 0;
    edu27_u32 index;
    edu27_u64 setup_value = 0;
    edu27_u64 compute_value = 0;
    edu27_u64 barrier_value = 0;
    edu27_u64 reduce_value = 0;
    if (events == (const edu27_u8 *)0 || event_count > 4) return 0;
    for (index = 0; index < event_count; index = index + 1) {
        const edu27_u8 *event = events + index * 32U;
        edu27_u32 next = edu27_event_advance(stage, event);
        if (next == 0xffffffffU) return 0;
        if (next == 1U) setup_value = edu27_read64(event + 24);
        if (next == 2U) compute_value = edu27_read64(event + 24);
        if (next == 3U) barrier_value = edu27_read64(event + 24);
        if (next == 4U) reduce_value = edu27_read64(event + 24);
        stage = next;
    }
    return edu27_phase_prefix_valid(
        effective_workers, stage, result,
        setup_value, compute_value, barrier_value, reduce_value);
}
