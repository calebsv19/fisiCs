typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 edu22_queue_entry_valid(const u8 *bytes);
extern int printf(const char *format, ...);

enum {
    STATE_PENDING = 1,
    STATE_RUNNING = 2,
    STATE_COMPLETE = 3,
    STATE_FAILED = 4,
    STATE_CANCELLED = 5,
    FLAG_CANCEL_PENDING = 1,
    FLAG_CANCEL_RUNNING = 2,
    FLAG_RESUMED = 4,
    EVENT_CANCEL_REQUESTED = 2,
    EVENT_RESOURCE_GRANTED = 6,
    EVENT_RUNNING_PUBLISHED = 7,
    EVENT_TERMINAL_COMPLETE = 11,
    EVENT_TERMINAL_FAILED = 12,
    EVENT_TERMINAL_CANCELLED = 13,
    EVENT_PHASE_SETUP_COMPLETE = 15,
    EVENT_PHASE_COMPUTE_COMPLETE = 16,
    EVENT_PHASE_BARRIER_COMPLETE = 17,
    EVENT_PHASE_REDUCE_COMPLETE = 18,
    EVENT_RESUME_RESTORED = 19,
    REASON_INTERRUPTED = 6,
    REASON_CANCELLED = 3,
    REASON_TIMEOUT = 9
};

static void put16(u8 *bytes, u32 offset, u16 value) {
    bytes[offset] = (u8)value;
    bytes[offset + 1U] = (u8)(value >> 8U);
}

static void put32(u8 *bytes, u32 offset, u32 value) {
    bytes[offset] = (u8)value;
    bytes[offset + 1U] = (u8)(value >> 8U);
    bytes[offset + 2U] = (u8)(value >> 16U);
    bytes[offset + 3U] = (u8)(value >> 24U);
}

static void put64(u8 *bytes, u32 offset, u64 value) {
    u32 index;
    for (index = 0U; index < 8U; index = index + 1U) {
        bytes[offset + index] = (u8)(value >> (index * 8U));
    }
}

static u32 fnv(const u8 *bytes, u32 count) {
    u32 value = 0x811C9DC5U;
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
        value = (value ^ bytes[index]) * 0x01000193U;
    }
    return value;
}

static void copy512(u8 *destination, const u8 *source) {
    u32 index;
    for (index = 0U; index < 512U; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void finish(u8 *bytes) {
    put32(bytes, 508U, fnv(bytes, 508U));
}

static void begin_entry(u8 *bytes, u32 state, u32 flags) {
    u32 index;
    for (index = 0U; index < 512U; index = index + 1U) bytes[index] = 0U;
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '1';
    bytes[4] = '5'; bytes[5] = 'J';
    put32(bytes, 8U, 8U);
    put32(bytes, 12U, 36U);
    put64(bytes, 16U, 0xED36000000000001ULL);
    put32(bytes, 24U, state);
    put32(bytes, 28U, flags);
    put32(bytes, 32U, 1U);
    put32(bytes, 36U, 2U);
    put64(bytes, 40U, 0x6EC4E5DB9E1056CFULL);
    put64(bytes, 48U, 0x1E3C373BAF48FAF7ULL);
    put16(bytes, 56U, 3U);
    put16(bytes, 58U, 32U);
    bytes[61] = 12U;
    put16(bytes, 62U, 15U);
    put16(bytes, 492U, 4U);
    put32(bytes, 498U, 11U);
    put16(bytes, 502U, 104U);
    put32(bytes, 504U, 0xA1B2C3D4U);
}

static void add_event(u8 *bytes, u32 index, u16 kind, u32 state,
                      u32 reason, u64 value) {
    u32 offset = 104U + index * 32U;
    put16(bytes, offset, (u16)(index + 1U));
    put16(bytes, offset + 2U, 2U);
    put16(bytes, offset + 4U, kind);
    put64(bytes, offset + 8U, (u64)(200U + index));
    put32(bytes, offset + 16U, state);
    put32(bytes, offset + 20U, reason);
    put64(bytes, offset + 24U, value);
    bytes[60] = (u8)(index + 1U);
    put16(bytes, 488U, 2U);
}

static u64 budget_value(u32 requested, u32 effective, u32 consumed) {
    return (u64)requested | ((u64)effective << 16U) |
           ((u64)consumed << 32U);
}

static void begin_resumed_active(u8 *bytes, u32 state, u32 flags) {
    begin_entry(bytes, state, flags);
    put64(bytes, 72U, 0xABCDEF02ULL);
    if (state == STATE_RUNNING) {
        put64(bytes, 80U, 0x3000ULL);
        put64(bytes, 88U, 0x4000ULL);
        put64(bytes, 96U, 1000ULL);
    } else {
        put64(bytes, 72U, 1000ULL);
    }
    put16(bytes, 490U, 1U);
    put16(bytes, 494U, 4U);
    put16(bytes, 496U, 3U);
    add_event(bytes, 0U, EVENT_RESOURCE_GRANTED, STATE_RUNNING, 0U,
              1ULL | (1ULL << 32U));
    add_event(bytes, 1U, EVENT_RUNNING_PUBLISHED, STATE_RUNNING, 0U, 0ULL);
    add_event(bytes, 2U, EVENT_RESUME_RESTORED, STATE_RUNNING,
              3U, 0xED35000000000001ULL);
}

static void build_resumed_running(u8 *bytes) {
    begin_resumed_active(bytes, STATE_RUNNING, FLAG_RESUMED);
    finish(bytes);
}

static void build_resumed_complete(u8 *bytes) {
    begin_resumed_active(bytes, STATE_COMPLETE, FLAG_RESUMED);
    put64(bytes, 64U, 0x6EC4E5DB9E1056CFULL);
    put16(bytes, 496U, 4U);
    add_event(bytes, 3U, EVENT_PHASE_REDUCE_COMPLETE, STATE_RUNNING,
              4U, 0x6EC4E5DB9E1056CFULL);
    add_event(bytes, 4U, EVENT_TERMINAL_COMPLETE, STATE_COMPLETE, 0U, 0ULL);
    finish(bytes);
}

static void build_resumed_failed(u8 *bytes, u32 reason, u64 terminal_value) {
    begin_resumed_active(bytes, STATE_FAILED, FLAG_RESUMED);
    put64(bytes, 96U, (u64)reason);
    add_event(bytes, 3U, EVENT_TERMINAL_FAILED, STATE_FAILED,
              reason, terminal_value);
    finish(bytes);
}

static void build_resumed_cancelled(u8 *bytes) {
    u64 budget = budget_value(4U, 4U, 3U);
    begin_resumed_active(
        bytes, STATE_CANCELLED, FLAG_CANCEL_RUNNING | FLAG_RESUMED);
    put64(bytes, 96U, REASON_CANCELLED);
    add_event(bytes, 3U, EVENT_CANCEL_REQUESTED, STATE_RUNNING, 0U, budget);
    add_event(bytes, 4U, EVENT_TERMINAL_CANCELLED, STATE_CANCELLED,
              REASON_CANCELLED, budget);
    finish(bytes);
}

static void build_ordinary_complete(u8 *bytes) {
    begin_entry(bytes, STATE_COMPLETE, 0U);
    put64(bytes, 64U, 0x6EC4E5DB9E1056CFULL);
    put64(bytes, 72U, 1000ULL);
    put16(bytes, 490U, 1U);
    put16(bytes, 494U, 4U);
    put16(bytes, 496U, 4U);
    add_event(bytes, 0U, EVENT_RESOURCE_GRANTED, STATE_RUNNING, 0U,
              1ULL | (1ULL << 32U));
    add_event(bytes, 1U, EVENT_RUNNING_PUBLISHED, STATE_RUNNING, 0U, 0ULL);
    add_event(bytes, 2U, EVENT_PHASE_SETUP_COMPLETE, STATE_RUNNING,
              1U, 1ULL | (3ULL << 32U));
    add_event(bytes, 3U, EVENT_PHASE_COMPUTE_COMPLETE, STATE_RUNNING,
              2U, (3ULL << 48U) | 6ULL);
    add_event(bytes, 4U, EVENT_PHASE_BARRIER_COMPLETE, STATE_RUNNING,
              3U, 1ULL | (1ULL << 16U) | (3ULL << 48U));
    add_event(bytes, 5U, EVENT_PHASE_REDUCE_COMPLETE, STATE_RUNNING,
              4U, 0x6EC4E5DB9E1056CFULL);
    add_event(bytes, 6U, EVENT_TERMINAL_COMPLETE, STATE_COMPLETE, 0U, 0ULL);
    finish(bytes);
}

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, u64 actual, u64 expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ (u32)actual) * 16777619U;
    if (actual != expected) return (int)id;
    return 0;
}

int main(void) {
    u8 baseline[512];
    u8 candidate[512];
    u8 unaligned[513];
    u32 event;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (ACTUAL), (EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    build_resumed_running(baseline);
    RUN(1U, edu22_queue_entry_valid(baseline), 0ULL);
    build_resumed_complete(baseline);
    RUN(2U, edu22_queue_entry_valid(baseline), 0ULL);
    build_resumed_failed(baseline, REASON_INTERRUPTED, 0ULL);
    RUN(3U, edu22_queue_entry_valid(baseline), 0ULL);
    build_resumed_failed(baseline, REASON_TIMEOUT, 1000ULL);
    RUN(4U, edu22_queue_entry_valid(baseline), 0ULL);
    build_resumed_cancelled(baseline);
    RUN(5U, edu22_queue_entry_valid(baseline), 0ULL);
    build_ordinary_complete(baseline);
    RUN(6U, edu22_queue_entry_valid(baseline), 0ULL);
    build_resumed_complete(baseline);
    unaligned[0] = 0xA5U; copy512(unaligned + 1U, baseline);
    RUN(7U, edu22_queue_entry_valid(unaligned + 1U), 0ULL);

    build_resumed_running(candidate);
    put32(candidate, 28U, 0U); finish(candidate);
    RUN(8U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    event = 104U + 2U * 32U;
    put16(candidate, event + 4U, EVENT_RUNNING_PUBLISHED); finish(candidate);
    RUN(9U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put64(candidate, event + 24U, 0ULL); finish(candidate);
    RUN(10U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put32(candidate, event + 16U, STATE_FAILED); finish(candidate);
    RUN(11U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put32(candidate, event + 20U, 2U); finish(candidate);
    RUN(12U, edu22_queue_entry_valid(candidate), 1ULL);

    build_resumed_running(candidate);
    put16(candidate, event + 4U, EVENT_PHASE_SETUP_COMPLETE);
    put32(candidate, event + 20U, 1U);
    put64(candidate, event + 24U, 1ULL | (3ULL << 32U));
    add_event(candidate, 3U, EVENT_RESUME_RESTORED, STATE_RUNNING,
              3U, 0xED35000000000001ULL);
    finish(candidate);
    RUN(13U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    add_event(candidate, 3U, EVENT_RESUME_RESTORED, STATE_RUNNING,
              3U, 0xED35000000000001ULL);
    finish(candidate);
    RUN(14U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    add_event(candidate, 3U, EVENT_PHASE_SETUP_COMPLETE, STATE_RUNNING,
              1U, 1ULL | (3ULL << 32U));
    finish(candidate);
    RUN(15U, edu22_queue_entry_valid(candidate), 1ULL);

    build_resumed_complete(candidate);
    put16(candidate, 104U + 3U * 32U + 4U,
          EVENT_TERMINAL_COMPLETE);
    finish(candidate);
    RUN(16U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_complete(candidate);
    put64(candidate, 104U + 3U * 32U + 24U, 1ULL); finish(candidate);
    RUN(17U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_complete(candidate);
    put16(candidate, 496U, 3U); finish(candidate);
    RUN(18U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_complete(candidate);
    put64(candidate, 64U, 0ULL); finish(candidate);
    RUN(19U, edu22_queue_entry_valid(candidate), 1ULL);
    build_ordinary_complete(candidate);
    put16(candidate, 104U + 4U * 32U + 4U,
          EVENT_PHASE_REDUCE_COMPLETE);
    put32(candidate, 104U + 4U * 32U + 20U, 4U);
    put64(candidate, 104U + 4U * 32U + 24U,
          0x6EC4E5DB9E1056CFULL);
    finish(candidate);
    RUN(20U, edu22_queue_entry_valid(candidate), 1ULL);

    build_resumed_cancelled(candidate);
    put32(candidate, 28U, FLAG_RESUMED); finish(candidate);
    RUN(21U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_cancelled(candidate);
    put64(candidate, 104U + 3U * 32U + 24U,
          budget_value(4U, 4U, 2U)); finish(candidate);
    RUN(22U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_failed(candidate, REASON_TIMEOUT, 1000ULL);
    put64(candidate, 104U + 3U * 32U + 24U, 999ULL); finish(candidate);
    RUN(23U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_failed(candidate, REASON_TIMEOUT, 1000ULL);
    put64(candidate, 72U, 0ULL); finish(candidate);
    RUN(24U, edu22_queue_entry_valid(candidate), 1ULL);

    build_resumed_running(candidate);
    put16(candidate, 62U, 16U); finish(candidate);
    RUN(25U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put32(candidate, 8U, 7U); finish(candidate);
    RUN(26U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put16(candidate, 56U, 2U); finish(candidate);
    RUN(27U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put16(candidate, 488U, 3U); finish(candidate);
    RUN(28U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    candidate[104U + 2U * 32U + 7U] = 2U; finish(candidate);
    RUN(29U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    candidate[508] ^= 1U;
    RUN(30U, edu22_queue_entry_valid(candidate), 1ULL);

    /*
     * CHECKPOINT_RESUME_V1 requires a compact resumed trace to carry all four
     * observation flags. The frozen Entry-v8 validator currently accepts any
     * in-range Trace-v3 flag set, including zero; retain that implementation
     * truth here as an upstream policy observation.
     */
    build_resumed_running(candidate);
    put16(candidate, 62U, 0U); finish(candidate);
    RUN(31U, edu22_queue_entry_valid(candidate), 0ULL);
    build_resumed_running(candidate);
    put64(candidate, event + 24U, 1ULL); finish(candidate);
    RUN(32U, edu22_queue_entry_valid(candidate), 0ULL);
    build_resumed_running(candidate);
    put64(candidate, event + 24U, 0xFFFFFFFFFFFFFFFFULL); finish(candidate);
    RUN(33U, edu22_queue_entry_valid(candidate), 0ULL);

    build_resumed_running(candidate);
    put16(candidate, 496U, 2U); finish(candidate);
    RUN(34U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put16(candidate, 494U, 3U); finish(candidate);
    RUN(35U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put32(candidate, 498U, 0U); finish(candidate);
    RUN(36U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put16(candidate, 502U, 103U); finish(candidate);
    RUN(37U, edu22_queue_entry_valid(candidate), 1ULL);
    build_resumed_running(candidate);
    put32(candidate, 504U, 0U); finish(candidate);
    RUN(38U, edu22_queue_entry_valid(candidate), 1ULL);

    printf(
        "OS-POST-EDU19 edu36-resume snapshot=d0e429b vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
