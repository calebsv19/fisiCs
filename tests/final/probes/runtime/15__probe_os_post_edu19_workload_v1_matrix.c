typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern int edu32_workload_valid(const u8 *bytes, u64 length);
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

static void copy104(u8 *destination, const u8 *source) {
    u32 index;
    for (index = 0U; index < 104U; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void build_default(u8 *bytes) {
    u32 index;
    for (index = 0U; index < 104U; index = index + 1U) bytes[index] = 0U;
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '3';
    bytes[4] = '2'; bytes[5] = 'W'; bytes[6] = '1';
    put16(bytes, 8U, 1U);
    put16(bytes, 10U, 1U);
    put32(bytes, 12U, 104U);
    put64(bytes, 16U, 0x3FF0000000000000ULL);
    put64(bytes, 24U, 0x3FD0000000000000ULL);
    put64(bytes, 32U, 0x3FA0000000000000ULL);
    put64(bytes, 40U, 0x4000000000000000ULL);
    put64(bytes, 48U, 0xBFC0000000000000ULL);
    put64(bytes, 56U, 0x3F90000000000000ULL);
    put64(bytes, 64U, 64ULL);
    put64(bytes, 72U, 0x9E3779B97F4A7C15ULL);
    put64(bytes, 80U, 0x4054800000000000ULL);
    put64(bytes, 88U, 0x403A800000000000ULL);
    put64(bytes, 96U, 0x6EC4E5DB9E1056CFULL);
}

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, int actual, int expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ (u32)actual) * 16777619U;
    if (actual != expected) return (int)id;
    return 0;
}

int main(void) {
    u8 baseline[104];
    u8 candidate[104];
    u8 unaligned[105];
    u32 field;
    int failure;

    build_default(baseline);
#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (ACTUAL), (EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    RUN(1U, edu32_workload_valid(baseline, 104ULL), 1);
    RUN(2U, edu32_workload_valid(baseline, 104ULL), 1);
    RUN(3U, edu32_workload_valid((const u8 *)0, 104ULL), 0);
    RUN(4U, edu32_workload_valid(baseline, 103ULL), 0);
    RUN(5U, edu32_workload_valid(baseline, 105ULL), 0);

    copy104(candidate, baseline); candidate[0] = 'X';
    RUN(6U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); put16(candidate, 8U, 2U);
    RUN(7U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); put16(candidate, 10U, 2U);
    RUN(8U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); put32(candidate, 12U, 103U);
    RUN(9U, edu32_workload_valid(candidate, 104ULL), 0);

    for (field = 0U; field < 6U; field = field + 1U) {
        copy104(candidate, baseline);
        put64(candidate, 16U + field * 8U, 0x7FF0000000000000ULL);
        RUN(10U + field, edu32_workload_valid(candidate, 104ULL), 0);
    }

    copy104(candidate, baseline); put64(candidate, 64U, 0ULL);
    RUN(16U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); put64(candidate, 64U, 65537ULL);
    RUN(17U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); candidate[80] ^= 1U;
    RUN(18U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); candidate[88] ^= 1U;
    RUN(19U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); candidate[96] ^= 1U;
    RUN(20U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline); candidate[72] ^= 1U;
    RUN(21U, edu32_workload_valid(candidate, 104ULL), 0);
    copy104(candidate, baseline);
    put64(candidate, 16U, 0x3FF8000000000000ULL);
    RUN(22U, edu32_workload_valid(candidate, 104ULL), 0);

    unaligned[0] = 0xA5U;
    copy104(unaligned + 1U, baseline);
    RUN(23U, edu32_workload_valid(unaligned + 1U, 104ULL), 1);

    copy104(candidate, baseline);
    put64(candidate, 64U, 1ULL);
    put64(candidate, 72U, 0x0123456789ABCDEFULL);
    put64(candidate, 80U, 0x3FF4800000000000ULL);
    put64(candidate, 88U, 0x3FFE400000000000ULL);
    put64(candidate, 96U, 0xE91E7B07A82F0AD9ULL);
    RUN(24U, edu32_workload_valid(candidate, 104ULL), 1);

    printf(
        "OS-POST-EDU19 workload-v1 snapshot=274f955 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
