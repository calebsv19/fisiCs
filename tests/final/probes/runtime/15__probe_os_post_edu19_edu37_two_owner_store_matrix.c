typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern int edu37_checkpoint_storage_valid(const u8 *lanes);
extern u64 edu37_checkpoint_lane_select(
    const u8 *lanes, const u8 *entries, u32 target_slot,
    u32 target_generation, u64 target_request);
extern int printf(const char *format, ...);

enum {
    STATE_RUNNING = 2,
    STATE_COMPLETE = 3,
    STATE_FAILED = 4,
    REASON_INTERRUPTED = 6
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

static void zero_bytes(u8 *bytes, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) bytes[index] = 0U;
}

static void copy_bytes(u8 *destination, const u8 *source, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void finish_snapshot(u8 *bytes) {
    put32(bytes, 40U, fnv(bytes + 80U, 104U));
    put32(bytes, 232U, fnv(bytes + 80U, 152U));
    put32(bytes, 236U, fnv(bytes, 236U));
    put32(bytes, 508U, fnv(bytes, 508U));
}

static void build_snapshot(
    u8 *bytes, u32 slot, u32 generation, u64 request) {
    u32 index;
    u64 expected_zero = 0x4054800000000000ULL;
    u64 expected_one = 0x403A800000000000ULL;
    zero_bytes(bytes, 512U);
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '3';
    bytes[4] = '5'; bytes[5] = 'C'; bytes[6] = 'P';
    put16(bytes, 8U, 1U);
    put16(bytes, 10U, 1U);
    put32(bytes, 12U, 240U);
    put32(bytes, 16U, slot);
    put32(bytes, 20U, generation);
    put64(bytes, 24U, request);
    put32(bytes, 32U, 11U);
    put16(bytes, 36U, 104U);
    put16(bytes, 38U, 3U);
    put16(bytes, 44U, 152U);
    put16(bytes, 46U, 3U);
    put64(bytes, 48U, 0x3156504D49534445ULL);
    put64(bytes, 56U, 0x1E3C373BAF48FAF7ULL);
    put64(bytes, 64U, 1000ULL);
    put16(bytes, 72U, 4U);
    put16(bytes, 74U, 4U);
    put16(bytes, 76U, 1U);
    bytes[80] = 'E'; bytes[81] = 'D'; bytes[82] = 'U'; bytes[83] = '3';
    bytes[84] = '2'; bytes[85] = 'W'; bytes[86] = '1';
    put16(bytes, 88U, 1U);
    put16(bytes, 90U, 1U);
    put32(bytes, 92U, 104U);
    put64(bytes, 96U, 0x3FF0000000000000ULL);
    put64(bytes, 104U, 0x3FD0000000000000ULL);
    put64(bytes, 112U, 0x3FA0000000000000ULL);
    put64(bytes, 120U, 0x4000000000000000ULL);
    put64(bytes, 128U, 0xBFC0000000000000ULL);
    put64(bytes, 136U, 0x3F90000000000000ULL);
    put64(bytes, 144U, 64ULL);
    put64(bytes, 152U, 0x9E3779B97F4A7C15ULL);
    put64(bytes, 160U, expected_zero);
    put64(bytes, 168U, expected_one);
    put64(bytes, 176U, 0x6EC4E5DB9E1056CFULL);
    for (index = 0U; index < 3U; index = index + 1U) {
        put64(bytes, 184U + index * 8U, expected_zero);
        put64(bytes, 208U + index * 8U, expected_one);
    }
    finish_snapshot(bytes);
}

static void build_entry(
    u8 *bytes, u32 generation, u64 request, u32 state,
    u64 reason, u16 consumed) {
    zero_bytes(bytes, 512U);
    bytes[0] = 'E';
    put32(bytes, 12U, generation);
    put64(bytes, 16U, request);
    put32(bytes, 24U, state);
    put64(bytes, 96U, reason);
    put16(bytes, 496U, consumed);
}

static void build_two(u8 *lanes) {
    build_snapshot(lanes, 0U, 11U, 0xED37000000000001ULL);
    build_snapshot(lanes + 512U, 1U, 12U, 0xED37000000000002ULL);
}

static void build_entries(u8 *entries) {
    zero_bytes(entries, 4096U);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_COMPLETE, 0ULL, 4U);
    build_entry(entries + 512U, 12U, 0xED37000000000002ULL,
                STATE_COMPLETE, 0ULL, 4U);
}

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, u64 actual, u64 expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ (u32)actual) * 16777619U;
    digest = (digest ^ (u32)(actual >> 32U)) * 16777619U;
    if (actual != expected) return (int)id;
    return 0;
}

int main(void) {
    u8 lanes[1024];
    u8 candidate[1024];
    u8 entries[4096];
    u8 unaligned_lanes[1025];
    u8 unaligned_entries[4097];
    const u64 failure_lane = 0xFFFFFFFFFFFFFFFFULL;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    zero_bytes(lanes, 1024U);
    RUN(1U, edu37_checkpoint_storage_valid(lanes), 1U);
    build_snapshot(lanes, 0U, 11U, 0xED37000000000001ULL);
    zero_bytes(lanes + 512U, 512U);
    RUN(2U, edu37_checkpoint_storage_valid(lanes), 1U);
    zero_bytes(lanes, 512U);
    build_snapshot(lanes + 512U, 0U, 11U, 0xED37000000000001ULL);
    RUN(3U, edu37_checkpoint_storage_valid(lanes), 1U);
    build_two(lanes);
    RUN(4U, edu37_checkpoint_storage_valid(lanes), 1U);

    build_snapshot(lanes + 512U, 0U, 11U, 0xED37000000000001ULL);
    RUN(5U, edu37_checkpoint_storage_valid(lanes), 0U);
    build_two(lanes);
    build_snapshot(lanes + 512U, 0U, 12U, 0xED37000000000001ULL);
    RUN(6U, edu37_checkpoint_storage_valid(lanes), 1U);
    build_snapshot(lanes + 512U, 0U, 11U, 0xED37000000000002ULL);
    RUN(7U, edu37_checkpoint_storage_valid(lanes), 1U);

    zero_bytes(lanes, 1024U); lanes[100U] = 1U;
    RUN(8U, edu37_checkpoint_storage_valid(lanes), 0U);
    build_two(lanes); lanes[508U] ^= 1U;
    RUN(9U, edu37_checkpoint_storage_valid(lanes), 0U);
    build_two(lanes); lanes[80U] = 'X'; finish_snapshot(lanes);
    RUN(10U, edu37_checkpoint_storage_valid(lanes), 0U);
    build_two(lanes); lanes[512U + 80U] = 'X';
    finish_snapshot(lanes + 512U);
    RUN(11U, edu37_checkpoint_storage_valid(lanes), 0U);
    build_two(lanes);
    unaligned_lanes[0] = 0xA5U;
    copy_bytes(unaligned_lanes + 1U, lanes, 1024U);
    RUN(12U, edu37_checkpoint_storage_valid(unaligned_lanes + 1U), 1U);

    build_two(lanes); build_entries(entries);
    RUN(13U, edu37_checkpoint_lane_select(
        lanes, entries, 0U, 11U, 0xED37000000000001ULL), 0U);
    RUN(14U, edu37_checkpoint_lane_select(
        lanes, entries, 1U, 12U, 0xED37000000000002ULL), 1U);

    zero_bytes(lanes, 512U);
    build_snapshot(lanes + 512U, 1U, 12U, 0xED37000000000002ULL);
    RUN(15U, edu37_checkpoint_lane_select(
        lanes, entries, 1U, 12U, 0xED37000000000002ULL), 1U);
    RUN(16U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_snapshot(lanes, 0U, 11U, 0xED37000000000001ULL);
    zero_bytes(lanes + 512U, 512U);
    RUN(17U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 1U);

    build_two(lanes); build_entries(entries);
    RUN(18U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_RUNNING, 0ULL, 0U);
    RUN(19U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 1U);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_FAILED, REASON_INTERRUPTED, 3U);
    RUN(20U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 1U);
    build_entry(entries + 512U, 12U, 0xED37000000000002ULL,
                STATE_RUNNING, 0ULL, 0U);
    RUN(21U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), failure_lane);

    build_entries(entries);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_FAILED, REASON_INTERRUPTED, 2U);
    RUN(22U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_FAILED, 5ULL, 3U);
    RUN(23U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_FAILED, REASON_INTERRUPTED, 4U);
    RUN(24U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);

    zero_bytes(entries, 512U);
    RUN(25U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_entries(entries); put32(entries, 12U, 99U);
    RUN(26U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_entries(entries); put64(entries, 16U, 99ULL);
    RUN(27U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);

    build_two(lanes); build_entries(entries);
    put32(lanes, 16U, 8U); finish_snapshot(lanes);
    RUN(28U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), failure_lane);
    build_two(lanes); put32(lanes + 512U, 16U, 8U);
    finish_snapshot(lanes + 512U);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_RUNNING, 0ULL, 0U);
    RUN(29U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), failure_lane);

    build_two(lanes); build_entries(entries);
    zero_bytes(entries, 4096U);
    RUN(30U, edu37_checkpoint_lane_select(
        lanes, entries, 0U, 11U, 0xED37000000000001ULL), 0U);
    build_entry(entries, 11U, 0xED37000000000001ULL,
                STATE_RUNNING, 0ULL, 0U);
    build_entry(entries + 512U, 12U, 0xED37000000000002ULL,
                STATE_RUNNING, 0ULL, 0U);
    RUN(31U, edu37_checkpoint_lane_select(
        lanes, entries, 1U, 12U, 0xED37000000000002ULL), 1U);

    build_two(lanes); build_entries(entries);
    unaligned_lanes[0] = 0xA5U;
    unaligned_entries[0] = 0x5AU;
    copy_bytes(unaligned_lanes + 1U, lanes, 1024U);
    copy_bytes(unaligned_entries + 1U, entries, 4096U);
    RUN(32U, edu37_checkpoint_lane_select(
        unaligned_lanes + 1U, unaligned_entries + 1U,
        2U, 13U, 0xED37000000000003ULL), 0U);

    build_two(lanes); build_entries(entries);
    zero_bytes(lanes + 512U, 512U);
    RUN(33U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 1U);
    zero_bytes(lanes, 1024U);
    RUN(34U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_two(lanes);
    build_entry(entries, 99U, 0xED37000000000001ULL,
                STATE_FAILED, REASON_INTERRUPTED, 3U);
    RUN(35U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);
    build_entries(entries);
    build_entry(entries, 11U, 99ULL,
                STATE_FAILED, REASON_INTERRUPTED, 3U);
    RUN(36U, edu37_checkpoint_lane_select(
        lanes, entries, 2U, 13U, 0xED37000000000003ULL), 0U);

    printf(
        "OS-POST-EDU19 edu37-two-owner snapshot=30e34df vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
