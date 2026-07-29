typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 edu15_queue_meta_valid(
    const u8 *bytes, u64 entry_lba, u64 entry_count);
extern u64 edu22_queue_entry_valid(const u8 *bytes);
extern u64 edu15_queue_entry_action(
    const u8 *bytes, u64 cpu_count, u64 free_pages);
extern int printf(const char *format, ...);

enum {
    STATE_PENDING = 1,
    STATE_RUNNING = 2,
    STATE_COMPLETE = 3,
    STATE_FAILED = 4,
    STATE_CANCELLED = 5
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
    for (index = 0; index < 8U; index = index + 1U) {
        bytes[offset + index] = (u8)(value >> (index * 8U));
    }
}

static u32 fnv(const u8 *bytes, u32 count) {
    u32 value = 0x811C9DC5U;
    u32 index;
    for (index = 0; index < count; index = index + 1U) {
        value = (value ^ bytes[index]) * 0x01000193U;
    }
    return value;
}

static void zero512(u8 *bytes) {
    u32 index;
    for (index = 0; index < 512U; index = index + 1U) bytes[index] = 0U;
}

static void copy512(u8 *destination, const u8 *source) {
    u32 index;
    for (index = 0; index < 512U; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void finish(u8 *bytes) {
    put32(bytes, 508U, fnv(bytes, 508U));
}

static void build_meta(u8 *bytes) {
    zero512(bytes);
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '1';
    bytes[4] = '5'; bytes[5] = 'Q';
    put32(bytes, 8U, 3U);
    put32(bytes, 12U, 8U);
    put32(bytes, 16U, 120U);
    put32(bytes, 24U, 2U);
    put32(bytes, 28U, 512U);
    put32(bytes, 32U, 1U);
    put32(bytes, 36U, 32U);
    put32(bytes, 40U, 12U);
    finish(bytes);
}

static void begin_entry(u8 *bytes, u32 state) {
    zero512(bytes);
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '1';
    bytes[4] = '5'; bytes[5] = 'J';
    put32(bytes, 8U, 2U);
    put32(bytes, 12U, 4U);
    put64(bytes, 16U, 0xED22000000000001ULL);
    put32(bytes, 24U, state);
    put32(bytes, 32U, 2U);
    put32(bytes, 36U, 2U);
    put64(bytes, 40U, 0x6EC4E5DB9E1056CFULL);
    put64(bytes, 48U, 0x1E3C373BAF48FAF7ULL);
    put16(bytes, 56U, 1U);
    put16(bytes, 58U, 32U);
    bytes[61] = 12U;
}

static void add_event(
    u8 *bytes, u32 index, u16 sequence, u16 epoch, u16 kind,
    u8 source, u64 tick, u32 state) {
    u32 offset = 104U + index * 32U;
    put16(bytes, offset, sequence);
    put16(bytes, offset + 2U, epoch);
    put16(bytes, offset + 4U, kind);
    bytes[offset + 7U] = source;
    put64(bytes, offset + 8U, tick);
    put32(bytes, offset + 16U, state);
    bytes[60] = (u8)(index + 1U);
    put16(bytes, 488U, epoch);
}

static void build_pending(u8 *bytes) {
    begin_entry(bytes, STATE_PENDING);
    finish(bytes);
}

static void build_running(u8 *bytes, int with_event) {
    begin_entry(bytes, STATE_RUNNING);
    put64(bytes, 72U, 1ULL);
    put64(bytes, 80U, 2ULL);
    put64(bytes, 88U, 3ULL);
    if (with_event) add_event(bytes, 0U, 1U, 1U, 7U, 0U, 100ULL, 2U);
    finish(bytes);
}

static void build_terminal(u8 *bytes, u32 state, u16 event_kind) {
    begin_entry(bytes, state);
    if (state == STATE_COMPLETE) {
        put64(bytes, 64U, 0x6EC4E5DB9E1056CFULL);
    } else {
        put64(bytes, 96U, (u64)state);
    }
    if (state == STATE_CANCELLED) put32(bytes, 28U, 1U);
    add_event(bytes, 0U, 1U, 1U, event_kind, 0U, 100ULL, state);
    finish(bytes);
}

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, u64 actual, u64 expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ (u32)actual) * 16777619U;
    return actual == expected ? 0 : (int)id;
}

int main(void) {
    u8 baseline[512];
    u8 candidate[512];
    u8 unaligned[513];
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (ACTUAL), (EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    build_meta(baseline);
    RUN(1U, edu15_queue_meta_valid(baseline, 120ULL, 8ULL), 0ULL);
    RUN(2U, edu15_queue_meta_valid(baseline, 119ULL, 8ULL), 1ULL);
    RUN(3U, edu15_queue_meta_valid(baseline, 120ULL, 7ULL), 1ULL);
    copy512(candidate, baseline); candidate[0] ^= 1U;
    RUN(4U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); put32(candidate, 8U, 2U); finish(candidate);
    RUN(5U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); put32(candidate, 20U, 1U); finish(candidate);
    RUN(6U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); put32(candidate, 24U, 3U); finish(candidate);
    RUN(7U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); put32(candidate, 28U, 511U); finish(candidate);
    RUN(8U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); put32(candidate, 32U, 2U); finish(candidate);
    RUN(9U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); put32(candidate, 36U, 31U); finish(candidate);
    RUN(10U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); put32(candidate, 40U, 11U); finish(candidate);
    RUN(11U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);
    copy512(candidate, baseline); candidate[508] ^= 1U;
    RUN(12U, edu15_queue_meta_valid(candidate, 120ULL, 8ULL), 1ULL);

    build_pending(baseline);
    RUN(13U, edu22_queue_entry_valid(baseline), 0ULL);
    build_running(baseline, 0);
    RUN(14U, edu22_queue_entry_valid(baseline), 0ULL);
    build_running(baseline, 1);
    RUN(15U, edu22_queue_entry_valid(baseline), 0ULL);
    build_terminal(baseline, STATE_COMPLETE, 11U);
    RUN(16U, edu22_queue_entry_valid(baseline), 0ULL);
    build_terminal(baseline, STATE_FAILED, 12U);
    RUN(17U, edu22_queue_entry_valid(baseline), 0ULL);
    build_terminal(baseline, STATE_CANCELLED, 13U);
    RUN(18U, edu22_queue_entry_valid(baseline), 0ULL);
    copy512(unaligned + 1U, baseline); unaligned[0] = 0xA5U;
    RUN(19U, edu22_queue_entry_valid(unaligned + 1U), 0ULL);

    build_pending(baseline);
    copy512(candidate, baseline); candidate[0] ^= 1U;
    RUN(20U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 8U, 1U); finish(candidate);
    RUN(21U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 12U, 0U); finish(candidate);
    RUN(22U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 12U, 9U); finish(candidate);
    RUN(23U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); candidate[508] ^= 1U;
    RUN(24U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 24U, 0U); finish(candidate);
    RUN(25U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 24U, 6U); finish(candidate);
    RUN(26U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 28U, 2U); finish(candidate);
    RUN(27U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 40U, 0ULL); finish(candidate);
    RUN(28U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 48U, 0ULL); finish(candidate);
    RUN(29U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 56U, 2U); finish(candidate);
    RUN(30U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 58U, 31U); finish(candidate);
    RUN(31U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); candidate[61] = 11U; finish(candidate);
    RUN(32U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 62U, 16U); finish(candidate);
    RUN(33U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); candidate[60] = 13U; finish(candidate);
    RUN(34U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); candidate[104] = 1U; finish(candidate);
    RUN(35U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); candidate[490] = 1U; finish(candidate);
    RUN(36U, edu22_queue_entry_valid(candidate), 1ULL);

    build_running(baseline, 1);
    copy512(candidate, baseline); put16(candidate, 104U, 0U); finish(candidate);
    RUN(37U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 108U, 0U); finish(candidate);
    RUN(38U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 108U, 15U); finish(candidate);
    RUN(39U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); candidate[111] = 2U; finish(candidate);
    RUN(40U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 120U, 6U); finish(candidate);
    RUN(41U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 488U, 2U); finish(candidate);
    RUN(42U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 108U, 6U); finish(candidate);
    RUN(43U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 64U, 1ULL); finish(candidate);
    RUN(44U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 72U, 0ULL); finish(candidate);
    RUN(45U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 80U, 0ULL); finish(candidate);
    RUN(46U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 88U, 0ULL); finish(candidate);
    RUN(47U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 96U, 1ULL); finish(candidate);
    RUN(48U, edu22_queue_entry_valid(candidate), 1ULL);

    build_terminal(baseline, STATE_COMPLETE, 11U);
    copy512(candidate, baseline); candidate[60] = 0U;
    put16(candidate, 488U, 0U); finish(candidate);
    RUN(49U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 108U, 12U); finish(candidate);
    RUN(50U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 64U, 0ULL); finish(candidate);
    RUN(51U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 72U, 1ULL); finish(candidate);
    RUN(52U, edu22_queue_entry_valid(candidate), 1ULL);

    build_terminal(baseline, STATE_FAILED, 12U);
    copy512(candidate, baseline); put16(candidate, 108U, 11U); finish(candidate);
    RUN(53U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 96U, 0ULL); finish(candidate);
    RUN(54U, edu22_queue_entry_valid(candidate), 1ULL);

    build_terminal(baseline, STATE_CANCELLED, 13U);
    copy512(candidate, baseline); put16(candidate, 108U, 12U); finish(candidate);
    RUN(55U, edu22_queue_entry_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 96U, 0ULL); finish(candidate);
    RUN(56U, edu22_queue_entry_valid(candidate), 1ULL);

    build_pending(baseline);
    RUN(57U, edu15_queue_entry_action(baseline, 2ULL, 2ULL), 0ULL);
    put32(baseline, 28U, 1U);
    RUN(58U, edu15_queue_entry_action(baseline, 2ULL, 2ULL), 3ULL);
    put32(baseline, 28U, 0U);
    RUN(59U, edu15_queue_entry_action(baseline, 1ULL, 2ULL), 2ULL);
    RUN(60U, edu15_queue_entry_action(baseline, 2ULL, 1ULL), 2ULL);
    put32(baseline, 32U, 1U);
    RUN(61U, edu15_queue_entry_action(baseline, 2ULL, 2ULL), 2ULL);
    put32(baseline, 32U, 2U); put32(baseline, 36U, 1U);
    RUN(62U, edu15_queue_entry_action(baseline, 2ULL, 2ULL), 2ULL);
    put32(baseline, 36U, 2U); put32(baseline, 24U, STATE_RUNNING);
    RUN(63U, edu15_queue_entry_action(baseline, 2ULL, 2ULL), 1ULL);
    build_pending(baseline); baseline[0] ^= 1U;
    RUN(64U, edu22_queue_entry_valid(baseline), 1ULL);
    RUN(65U, edu15_queue_entry_action(baseline, 2ULL, 2ULL), 0ULL);

    printf(
        "OS-POST-EDU19 edu22-queue-v2 snapshot=37900ba vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
