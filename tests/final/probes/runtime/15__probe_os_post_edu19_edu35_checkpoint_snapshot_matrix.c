typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 edu35_checkpoint_snapshot_valid(const u8 *bytes);
extern int printf(const char *format, ...);

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

static void finish_whole(u8 *bytes) {
    put32(bytes, 508U, fnv(bytes, 508U));
}

static void finish_logical(u8 *bytes) {
    put32(bytes, 236U, fnv(bytes, 236U));
    finish_whole(bytes);
}

static void finish_state(u8 *bytes) {
    put32(bytes, 232U, fnv(bytes + 80U, 152U));
    finish_logical(bytes);
}

static void finish_all(u8 *bytes) {
    put32(bytes, 40U, fnv(bytes + 80U, 104U));
    finish_state(bytes);
}

static void build_default(u8 *bytes) {
    u32 index;
    u64 expected_zero = 0x4054800000000000ULL;
    u64 expected_one = 0x403A800000000000ULL;
    for (index = 0U; index < 512U; index = index + 1U) bytes[index] = 0U;
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '3';
    bytes[4] = '5'; bytes[5] = 'C'; bytes[6] = 'P';
    put16(bytes, 8U, 1U);
    put16(bytes, 10U, 1U);
    put32(bytes, 12U, 240U);
    put32(bytes, 16U, 3U);
    put32(bytes, 20U, 19U);
    put64(bytes, 24U, 0xED35000000000001ULL);
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
    put16(bytes, 76U, 2U);
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
    finish_all(bytes);
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
    int failure;

    build_default(baseline);
#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (ACTUAL), (EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    RUN(1U, edu35_checkpoint_snapshot_valid(baseline), 1ULL);
    RUN(2U, edu35_checkpoint_snapshot_valid(baseline), 1ULL);
    copy512(candidate, baseline); put32(candidate, 16U, 0U); finish_all(candidate);
    RUN(3U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 16U, 7U); finish_all(candidate);
    RUN(4U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);
    copy512(candidate, baseline); put64(candidate, 64U, 0ULL); finish_all(candidate);
    RUN(5U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);
    copy512(candidate, baseline);
    put64(candidate, 64U, 60000000000ULL); finish_all(candidate);
    RUN(6U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);
    copy512(candidate, baseline);
    put16(candidate, 72U, 3U); put16(candidate, 74U, 3U); finish_all(candidate);
    RUN(7U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);
    copy512(candidate, baseline); put16(candidate, 76U, 1U); finish_all(candidate);
    RUN(8U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);
    unaligned[0] = 0xA5U; copy512(unaligned + 1U, baseline);
    RUN(9U, edu35_checkpoint_snapshot_valid(unaligned + 1U), 1ULL);

    copy512(candidate, baseline);
    put64(candidate, 160U, 0ULL); put64(candidate, 168U, 0ULL);
    put64(candidate, 184U, 0ULL); put64(candidate, 192U, 0ULL);
    put64(candidate, 200U, 0ULL); put64(candidate, 208U, 0ULL);
    put64(candidate, 216U, 0ULL); put64(candidate, 224U, 0ULL);
    finish_all(candidate);
    RUN(10U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);
    copy512(candidate, baseline);
    put64(candidate, 160U, 0x7FF8000000000001ULL);
    put64(candidate, 168U, 0xFFFFFFFFFFFFFFFFULL);
    put64(candidate, 184U, 0x7FF8000000000001ULL);
    put64(candidate, 192U, 0x7FF8000000000001ULL);
    put64(candidate, 200U, 0x7FF8000000000001ULL);
    put64(candidate, 208U, 0xFFFFFFFFFFFFFFFFULL);
    put64(candidate, 216U, 0xFFFFFFFFFFFFFFFFULL);
    put64(candidate, 224U, 0xFFFFFFFFFFFFFFFFULL);
    finish_all(candidate);
    RUN(11U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);

    copy512(candidate, baseline); candidate[0] = 'X'; finish_all(candidate);
    RUN(12U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[7] = 1U; finish_all(candidate);
    RUN(13U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 8U, 2U); finish_all(candidate);
    RUN(14U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 10U, 2U); finish_all(candidate);
    RUN(15U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 12U, 239U); finish_all(candidate);
    RUN(16U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 16U, 8U); finish_all(candidate);
    RUN(17U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 20U, 0U); finish_all(candidate);
    RUN(18U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put64(candidate, 24U, 0ULL); finish_all(candidate);
    RUN(19U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 32U, 0U); finish_all(candidate);
    RUN(20U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 36U, 103U); finish_all(candidate);
    RUN(21U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 38U, 2U); finish_all(candidate);
    RUN(22U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);

    copy512(candidate, baseline); put32(candidate, 40U, 0U); finish_state(candidate);
    RUN(23U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 44U, 151U); finish_all(candidate);
    RUN(24U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 46U, 2U); finish_all(candidate);
    RUN(25U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline);
    put64(candidate, 48U, 0x3156504D49534444ULL); finish_all(candidate);
    RUN(26U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline);
    put64(candidate, 56U, 0x1E3C373BAF48FAF6ULL); finish_all(candidate);
    RUN(27U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline);
    put64(candidate, 64U, 60000000001ULL); finish_all(candidate);
    RUN(28U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline);
    put16(candidate, 72U, 2U); put16(candidate, 74U, 2U); finish_all(candidate);
    RUN(29U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline);
    put16(candidate, 72U, 5U); put16(candidate, 74U, 5U); finish_all(candidate);
    RUN(30U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 74U, 3U); finish_all(candidate);
    RUN(31U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 76U, 0U); finish_all(candidate);
    RUN(32U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 76U, 3U); finish_all(candidate);
    RUN(33U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 78U, 1U); finish_all(candidate);
    RUN(34U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);

    copy512(candidate, baseline); candidate[184] ^= 1U; finish_all(candidate);
    RUN(35U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[200] ^= 1U; finish_all(candidate);
    RUN(36U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[216] ^= 1U; finish_all(candidate);
    RUN(37U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);

    copy512(candidate, baseline); candidate[40] ^= 1U; finish_state(candidate);
    RUN(38U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[232] ^= 1U; finish_logical(candidate);
    RUN(39U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[236] ^= 1U; finish_whole(candidate);
    RUN(40U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[240] = 1U; finish_whole(candidate);
    RUN(41U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[400] = 1U; finish_whole(candidate);
    RUN(42U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[507] = 1U; finish_whole(candidate);
    RUN(43U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[508] ^= 1U;
    RUN(44U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);

    copy512(candidate, baseline); candidate[160] ^= 1U; finish_all(candidate);
    RUN(45U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);
    copy512(candidate, baseline);
    put64(candidate, 184U, 0x0123456789ABCDEFULL);
    put64(candidate, 192U, 0x0123456789ABCDEFULL);
    put64(candidate, 200U, 0x0123456789ABCDEFULL);
    finish_all(candidate);
    RUN(46U, edu35_checkpoint_snapshot_valid(candidate), 0ULL);

    /*
     * The frozen checkpoint validator binds the embedded bytes by checksum
     * and repeated partition evidence; the surrounding OS path owns complete
     * Workload-v1 semantic admission and authoritative correlation.
     */
    copy512(candidate, baseline); candidate[80] = 'X'; finish_all(candidate);
    RUN(47U, edu35_checkpoint_snapshot_valid(candidate), 1ULL);

    printf(
        "OS-POST-EDU19 edu35-checkpoint snapshot=5b39037 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
