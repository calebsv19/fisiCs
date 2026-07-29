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
    EVENT_CANCEL_REQUESTED = 2,
    EVENT_RESOURCE_GRANTED = 6,
    EVENT_RUNNING_PUBLISHED = 7,
    EVENT_TERMINAL_FAILED = 12,
    EVENT_TERMINAL_CANCELLED = 13,
    EVENT_PHASE_SETUP_COMPLETE = 15,
    REASON_CANCELLED = 3,
    REASON_BUDGET = 8,
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

static void begin_entry(u8 *bytes, u32 state, u32 requested_work) {
    u32 index;
    for (index = 0U; index < 512U; index = index + 1U) bytes[index] = 0U;
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '1';
    bytes[4] = '5'; bytes[5] = 'J';
    put32(bytes, 8U, 7U);
    put32(bytes, 12U, 19U);
    put64(bytes, 16U, 0xED34000000000001ULL);
    put32(bytes, 24U, state);
    put32(bytes, 32U, 1U);
    put32(bytes, 36U, 2U);
    put64(bytes, 40U, 0x6EC4E5DB9E1056CFULL);
    put64(bytes, 48U, 0x1E3C373BAF48FAF7ULL);
    put16(bytes, 56U, 2U);
    put16(bytes, 58U, 32U);
    bytes[61] = 12U;
    put16(bytes, 492U, (u16)requested_work);
    put32(bytes, 498U, 11U);
    put16(bytes, 502U, 104U);
    put32(bytes, 504U, 0xA1B2C3D4U);
}

static void add_event(u8 *bytes, u32 index, u16 kind, u32 state,
                      u32 reason, u64 value) {
    u32 offset = 104U + index * 32U;
    put16(bytes, offset, (u16)(index + 1U));
    put16(bytes, offset + 2U, 1U);
    put16(bytes, offset + 4U, kind);
    put64(bytes, offset + 8U, (u64)(100U + index));
    put32(bytes, offset + 16U, state);
    put32(bytes, offset + 20U, reason);
    put64(bytes, offset + 24U, value);
    bytes[60] = (u8)(index + 1U);
    put16(bytes, 488U, 1U);
}

static u64 budget_value(u32 requested, u32 effective, u32 consumed) {
    return (u64)requested | ((u64)effective << 16U) |
           ((u64)consumed << 32U);
}

static void build_pending(u8 *bytes, u64 duration) {
    begin_entry(bytes, STATE_PENDING, 4U);
    put64(bytes, 96U, duration);
    finish(bytes);
}

static void build_running(u8 *bytes, u64 duration) {
    begin_entry(bytes, STATE_RUNNING, 4U);
    put64(bytes, 72U, 0xABCDEF01ULL);
    put64(bytes, 80U, 0x1000ULL);
    put64(bytes, 88U, 0x2000ULL);
    put64(bytes, 96U, duration);
    put16(bytes, 490U, 1U);
    put16(bytes, 494U, 4U);
    add_event(bytes, 0U, EVENT_RESOURCE_GRANTED, STATE_RUNNING, 0U,
              1ULL | (1ULL << 32U));
    add_event(bytes, 1U, EVENT_RUNNING_PUBLISHED, STATE_RUNNING, 0U, 0ULL);
    finish(bytes);
}

static void build_timeout(u8 *bytes, u64 duration, u32 prefix) {
    u32 terminal_index;
    begin_entry(bytes, STATE_FAILED, 4U);
    put64(bytes, 72U, duration);
    put64(bytes, 96U, REASON_TIMEOUT);
    put16(bytes, 490U, 1U);
    put16(bytes, 494U, 4U);
    put16(bytes, 496U, (u16)prefix);
    add_event(bytes, 0U, EVENT_RESOURCE_GRANTED, STATE_RUNNING, 0U,
              1ULL | (1ULL << 32U));
    add_event(bytes, 1U, EVENT_RUNNING_PUBLISHED, STATE_RUNNING, 0U, 0ULL);
    terminal_index = 2U;
    if (prefix != 0U) {
        add_event(bytes, 2U, EVENT_PHASE_SETUP_COMPLETE, STATE_RUNNING, 1U,
                  1ULL | (3ULL << 32U));
        terminal_index = 3U;
    }
    add_event(bytes, terminal_index, EVENT_TERMINAL_FAILED, STATE_FAILED,
              REASON_TIMEOUT, duration);
    finish(bytes);
}

static void build_budget_failure(u8 *bytes, u64 duration) {
    begin_entry(bytes, STATE_FAILED, 1U);
    put64(bytes, 72U, duration);
    put64(bytes, 96U, REASON_BUDGET);
    put16(bytes, 490U, 1U);
    put16(bytes, 494U, 1U);
    put16(bytes, 496U, 1U);
    add_event(bytes, 0U, EVENT_RESOURCE_GRANTED, STATE_RUNNING, 0U,
              1ULL | (1ULL << 32U));
    add_event(bytes, 1U, EVENT_RUNNING_PUBLISHED, STATE_RUNNING, 0U, 0ULL);
    add_event(bytes, 2U, EVENT_PHASE_SETUP_COMPLETE, STATE_RUNNING, 1U,
              1ULL | (3ULL << 32U));
    add_event(bytes, 3U, EVENT_TERMINAL_FAILED, STATE_FAILED, REASON_BUDGET,
              budget_value(1U, 1U, 1U));
    finish(bytes);
}

static void build_pending_cancel(u8 *bytes, u64 duration) {
    begin_entry(bytes, STATE_CANCELLED, 4U);
    put32(bytes, 28U, FLAG_CANCEL_PENDING);
    put64(bytes, 72U, duration);
    put64(bytes, 96U, REASON_CANCELLED);
    add_event(bytes, 0U, EVENT_CANCEL_REQUESTED, STATE_PENDING, 0U, 4ULL);
    add_event(bytes, 1U, EVENT_TERMINAL_CANCELLED, STATE_CANCELLED,
              REASON_CANCELLED, budget_value(4U, 0U, 0U));
    finish(bytes);
}

static void build_running_cancel(u8 *bytes, u64 duration) {
    u64 budget = budget_value(4U, 4U, 0U);
    begin_entry(bytes, STATE_CANCELLED, 4U);
    put32(bytes, 28U, FLAG_CANCEL_RUNNING);
    put64(bytes, 72U, duration);
    put64(bytes, 96U, REASON_CANCELLED);
    put16(bytes, 490U, 1U);
    put16(bytes, 494U, 4U);
    add_event(bytes, 0U, EVENT_RESOURCE_GRANTED, STATE_RUNNING, 0U,
              1ULL | (1ULL << 32U));
    add_event(bytes, 1U, EVENT_RUNNING_PUBLISHED, STATE_RUNNING, 0U, 0ULL);
    add_event(bytes, 2U, EVENT_CANCEL_REQUESTED, STATE_RUNNING, 0U, budget);
    add_event(bytes, 3U, EVENT_TERMINAL_CANCELLED, STATE_CANCELLED,
              REASON_CANCELLED, budget);
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
    u32 terminal;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (ACTUAL), (EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    build_pending(baseline, 0ULL);
    RUN(1U, edu22_queue_entry_valid(baseline), 0ULL);
    build_pending(baseline, 1ULL);
    RUN(2U, edu22_queue_entry_valid(baseline), 0ULL);
    build_pending(baseline, 60000000000ULL);
    RUN(3U, edu22_queue_entry_valid(baseline), 0ULL);
    build_running(baseline, 1ULL);
    RUN(4U, edu22_queue_entry_valid(baseline), 0ULL);
    build_running(baseline, 60000000000ULL);
    RUN(5U, edu22_queue_entry_valid(baseline), 0ULL);
    build_timeout(baseline, 1ULL, 0U);
    RUN(6U, edu22_queue_entry_valid(baseline), 0ULL);
    build_timeout(baseline, 60000000000ULL, 0U);
    RUN(7U, edu22_queue_entry_valid(baseline), 0ULL);
    build_timeout(baseline, 1000ULL, 1U);
    RUN(8U, edu22_queue_entry_valid(baseline), 0ULL);
    build_budget_failure(baseline, 1000ULL);
    RUN(9U, edu22_queue_entry_valid(baseline), 0ULL);
    build_pending_cancel(baseline, 1000ULL);
    RUN(10U, edu22_queue_entry_valid(baseline), 0ULL);
    build_running_cancel(baseline, 1000ULL);
    RUN(11U, edu22_queue_entry_valid(baseline), 0ULL);
    build_timeout(baseline, 1000ULL, 1U);
    unaligned[0] = 0xA5U;
    copy512(unaligned + 1U, baseline);
    RUN(12U, edu22_queue_entry_valid(unaligned + 1U), 0ULL);

    build_pending(candidate, 60000000001ULL);
    RUN(13U, edu22_queue_entry_valid(candidate), 1ULL);
    build_running(candidate, 60000000001ULL);
    RUN(14U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 60000000001ULL, 0U);
    RUN(15U, edu22_queue_entry_valid(candidate), 1ULL);

    build_timeout(candidate, 0ULL, 0U);
    RUN(16U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 0U);
    terminal = 104U + 2U * 32U;
    put64(candidate, terminal + 24U, 999ULL);
    finish(candidate);
    RUN(17U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 0U);
    put32(candidate, terminal + 20U, REASON_BUDGET);
    finish(candidate);
    RUN(18U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 0U);
    put64(candidate, 96U, REASON_BUDGET);
    finish(candidate);
    RUN(19U, edu22_queue_entry_valid(candidate), 1ULL);

    /*
     * Frozen EDU-34 compares the terminal event reason with read32(p + 96).
     * Preserve its current admission of nonzero upper reason-slot bits so the
     * compiler probe reports implementation truth without normalizing policy.
     */
    build_timeout(candidate, 1000ULL, 0U);
    put32(candidate, 100U, 1U);
    finish(candidate);
    RUN(20U, edu22_queue_entry_valid(candidate), 0ULL);

    build_timeout(candidate, 1000ULL, 0U);
    put64(candidate, 80U, 1ULL);
    finish(candidate);
    RUN(21U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 0U);
    put64(candidate, 88U, 1ULL);
    finish(candidate);
    RUN(22U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 0U);
    put16(candidate, 104U + 32U + 4U, 10U);
    finish(candidate);
    RUN(23U, edu22_queue_entry_valid(candidate), 1ULL);

    build_pending(candidate, 1000ULL);
    put64(candidate, 72U, 1000ULL);
    put64(candidate, 96U, 0ULL);
    finish(candidate);
    RUN(24U, edu22_queue_entry_valid(candidate), 1ULL);
    build_running(candidate, 0ULL);
    RUN(25U, edu22_queue_entry_valid(candidate), 0ULL);
    build_running(candidate, 60000000001ULL);
    RUN(26U, edu22_queue_entry_valid(candidate), 1ULL);

    build_timeout(candidate, 1000ULL, 0U);
    candidate[508] ^= 1U;
    RUN(27U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 0U);
    put32(candidate, 8U, 6U);
    finish(candidate);
    RUN(28U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 0U);
    put32(candidate, 24U, STATE_COMPLETE);
    finish(candidate);
    RUN(29U, edu22_queue_entry_valid(candidate), 1ULL);
    build_timeout(candidate, 1000ULL, 1U);
    put16(candidate, 496U, 0U);
    finish(candidate);
    RUN(30U, edu22_queue_entry_valid(candidate), 1ULL);

    printf(
        "OS-POST-EDU19 edu34-deadline snapshot=bf95c67 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
