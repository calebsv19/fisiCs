typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

extern u64 edu27_expected_phase_value(u32 effective_workers, u32 phase_stage);
extern u32 edu27_event_advance(u32 current_stage, const u8 *event);
extern int edu27_phase_prefix_valid(
    u32 effective_workers, u32 phase_stage, u64 result,
    u64 setup_value, u64 compute_value,
    u64 barrier_value, u64 reduce_value);
extern int edu27_trace_prefix_valid(
    u32 effective_workers, const u8 *events,
    u32 event_count, u64 result);
extern int printf(const char *format, ...);

static void put16(u8 *p, u32 offset, u32 value) {
    p[offset] = (u8)value;
    p[offset + 1U] = (u8)(value >> 8);
}

static void put32(u8 *p, u32 offset, u32 value) {
    p[offset] = (u8)value;
    p[offset + 1U] = (u8)(value >> 8);
    p[offset + 2U] = (u8)(value >> 16);
    p[offset + 3U] = (u8)(value >> 24);
}

static void put64(u8 *p, u32 offset, u64 value) {
    u32 index;
    for (index = 0; index < 8U; index = index + 1U) {
        p[offset + index] = (u8)(value >> (index * 8U));
    }
}

static void zero_bytes(u8 *p, u32 count) {
    u32 index;
    for (index = 0; index < count; index = index + 1U) p[index] = 0U;
}

static void copy_bytes(u8 *to, const u8 *from, u32 count) {
    u32 index;
    for (index = 0; index < count; index = index + 1U) to[index] = from[index];
}

static void build_events(u8 *events, u32 workers) {
    u32 stage;
    zero_bytes(events, 128U);
    for (stage = 1U; stage <= 4U; stage = stage + 1U) {
        u8 *event = events + (stage - 1U) * 32U;
        put16(event, 0U, stage);
        put16(event, 2U, 1U);
        put16(event, 4U, 14U + stage);
        put32(event, 16U, 2U);
        put32(event, 20U, stage);
        put64(event, 24U, edu27_expected_phase_value(workers, stage));
    }
}

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, u64 actual, u64 expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ (u32)actual) * 16777619U;
    digest = (digest ^ (u32)(actual >> 32U)) * 16777619U;
    return actual == expected ? 0 : (int)id;
}

int main(void) {
    u8 events[128];
    u8 candidate[128];
    u8 unaligned[129];
    u32 workers;
    u32 stage;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    RUN(1U, edu27_expected_phase_value(1U, 1U), 1ULL | (3ULL << 32));
    RUN(2U, edu27_expected_phase_value(2U, 1U), 2ULL | (3ULL << 32));
    RUN(3U, edu27_expected_phase_value(1U, 2U), 6ULL | (3ULL << 48));
    RUN(4U, edu27_expected_phase_value(2U, 2U),
        3ULL | (1ULL << 16) | (3ULL << 32) | (3ULL << 48));
    RUN(5U, edu27_expected_phase_value(1U, 3U),
        1ULL | (1ULL << 16) | (3ULL << 48));
    RUN(6U, edu27_expected_phase_value(2U, 3U),
        2ULL | (2ULL << 16) | (1ULL << 32) | (3ULL << 48));
    RUN(7U, edu27_expected_phase_value(1U, 4U), 0x6EC4E5DB9E1056CFULL);
    RUN(8U, edu27_expected_phase_value(0U, 1U), ~0ULL);
    RUN(9U, edu27_expected_phase_value(3U, 1U), ~0ULL);
    RUN(10U, edu27_expected_phase_value(1U, 0U), ~0ULL);
    RUN(11U, edu27_expected_phase_value(1U, 5U), ~0ULL);

    for (workers = 1U; workers <= 2U; workers = workers + 1U) {
        build_events(events, workers);
        RUN(12U + (workers - 1U) * 5U,
            edu27_trace_prefix_valid(workers, events, 0U, 0ULL), 1U);
        for (stage = 1U; stage <= 3U; stage = stage + 1U) {
            RUN(12U + (workers - 1U) * 5U + stage,
                edu27_trace_prefix_valid(
                    workers, events, stage, 0ULL), 1U);
        }
        RUN(16U + (workers - 1U) * 5U,
            edu27_trace_prefix_valid(
                workers, events, 4U, 0x6EC4E5DB9E1056CFULL), 1U);
    }

    build_events(events, 2U);
    RUN(22U, edu27_event_advance(0U, events), 1U);
    RUN(23U, edu27_event_advance(1U, events + 32U), 2U);
    RUN(24U, edu27_event_advance(2U, events + 64U), 3U);
    RUN(25U, edu27_event_advance(3U, events + 96U), 4U);
    RUN(26U, edu27_event_advance(4U, events + 96U), 0xffffffffU);
    RUN(27U, edu27_event_advance(0U, (const u8 *)0), 0xffffffffU);

    copy_bytes(candidate, events, 128U); put16(candidate + 32U, 4U, 17U);
    RUN(28U, edu27_trace_prefix_valid(2U, candidate, 2U, 0ULL), 0U);
    copy_bytes(candidate, events, 128U); put32(candidate + 32U, 16U, 3U);
    RUN(29U, edu27_trace_prefix_valid(2U, candidate, 2U, 0ULL), 0U);
    copy_bytes(candidate, events, 128U); put32(candidate + 32U, 20U, 3U);
    RUN(30U, edu27_trace_prefix_valid(2U, candidate, 2U, 0ULL), 0U);
    copy_bytes(candidate, events, 128U); candidate[32U + 24U] ^= 1U;
    RUN(31U, edu27_trace_prefix_valid(2U, candidate, 2U, 0ULL), 0U);
    copy_bytes(candidate, events, 128U); candidate[64U + 24U] ^= 1U;
    RUN(32U, edu27_trace_prefix_valid(2U, candidate, 3U, 0ULL), 0U);
    copy_bytes(candidate, events, 128U); candidate[96U + 24U] ^= 1U;
    RUN(33U, edu27_trace_prefix_valid(
        2U, candidate, 4U, 0x6EC4E5DB9E1056CFULL), 0U);
    RUN(34U, edu27_trace_prefix_valid(2U, events, 4U, 0ULL), 0U);
    RUN(35U, edu27_trace_prefix_valid(0U, events, 1U, 0ULL), 0U);
    RUN(36U, edu27_trace_prefix_valid(2U, events, 5U, 0ULL), 0U);
    RUN(37U, edu27_trace_prefix_valid(
        2U, (const u8 *)0, 1U, 0ULL), 0U);

    RUN(38U, edu27_phase_prefix_valid(
        1U, 0U, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL), 1U);
    RUN(39U, edu27_phase_prefix_valid(
        1U, 0U, 1ULL, 0ULL, 0ULL, 0ULL, 0ULL), 0U);
    RUN(40U, edu27_phase_prefix_valid(
        1U, 1U, 0ULL,
        edu27_expected_phase_value(1U, 1U), 0ULL, 0ULL, 0ULL), 1U);
    RUN(41U, edu27_phase_prefix_valid(
        1U, 1U, 0ULL,
        edu27_expected_phase_value(1U, 1U), 1ULL, 0ULL, 0ULL), 0U);
    RUN(42U, edu27_phase_prefix_valid(
        2U, 4U, 0x6EC4E5DB9E1056CFULL,
        edu27_expected_phase_value(2U, 1U),
        edu27_expected_phase_value(2U, 2U),
        edu27_expected_phase_value(2U, 3U),
        edu27_expected_phase_value(2U, 4U)), 1U);

    unaligned[0U] = 0xA5U; copy_bytes(unaligned + 1U, events, 128U);
    RUN(43U, edu27_trace_prefix_valid(
        2U, unaligned + 1U, 4U, 0x6EC4E5DB9E1056CFULL), 1U);

    printf(
        "OS-POST-EDU19 edu27-phase-execution snapshot=9c9e2b0 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
