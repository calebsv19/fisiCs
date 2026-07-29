typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 edu21_control_validate_request(const u8 *bytes);
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

static void copy64(u8 *destination, const u8 *source) {
    u32 index;
    for (index = 0; index < 64U; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void build(u8 *bytes, u8 operation, u16 payload) {
    u32 index;
    for (index = 0; index < 64U; index = index + 1U) bytes[index] = 0U;
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '2';
    bytes[4] = '1'; bytes[5] = 'R'; bytes[6] = 'Q';
    put16(bytes, 8U, 1U);
    bytes[10] = operation;
    put16(bytes, 12U, payload);
    put64(bytes, 16U, 0xED21000000000001ULL);
    if (operation == 3U && payload == 8U) {
        put32(bytes, 24U, 7U);
        put32(bytes, 28U, 3U);
    }
    put32(bytes, 60U, fnv(bytes, 60U));
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
    u8 baseline[64];
    u8 candidate[64];
    u8 unaligned[65];
    u32 operation;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (ACTUAL), (EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    for (operation = 1U; operation <= 6U; operation = operation + 1U) {
        build(baseline, (u8)operation, operation == 3U ? 8U : 0U);
        RUN(operation, edu21_control_validate_request(baseline), 0ULL);
    }
    build(baseline, 3U, 8U);
    copy64(unaligned + 1U, baseline);
    unaligned[0] = 0xA5U;
    RUN(7U, edu21_control_validate_request(unaligned + 1U), 0ULL);

    copy64(candidate, baseline); candidate[0] ^= 1U;
    RUN(8U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); put16(candidate, 8U, 2U);
    RUN(9U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); candidate[11] = 1U;
    RUN(10U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); put16(candidate, 14U, 1U);
    RUN(11U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); put16(candidate, 12U, 33U);
    RUN(12U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); put64(candidate, 16U, 0ULL);
    RUN(13U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); candidate[56] = 1U;
    RUN(14U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); candidate[57] = 1U;
    RUN(15U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); candidate[58] = 1U;
    RUN(16U, edu21_control_validate_request(candidate), 1ULL);
    copy64(candidate, baseline); candidate[59] = 1U;
    RUN(17U, edu21_control_validate_request(candidate), 1ULL);

    copy64(candidate, baseline); candidate[24] ^= 1U;
    RUN(18U, edu21_control_validate_request(candidate), 2ULL);
    copy64(candidate, baseline); candidate[60] ^= 1U;
    RUN(19U, edu21_control_validate_request(candidate), 2ULL);
    copy64(candidate, baseline); candidate[10] = 0U;
    put32(candidate, 60U, fnv(candidate, 60U));
    RUN(20U, edu21_control_validate_request(candidate), 3ULL);
    copy64(candidate, baseline); candidate[10] = 7U;
    put16(candidate, 12U, 0U); put32(candidate, 60U, fnv(candidate, 60U));
    RUN(21U, edu21_control_validate_request(candidate), 3ULL);
    copy64(candidate, baseline); candidate[10] = 8U;
    put16(candidate, 12U, 4U); put32(candidate, 60U, fnv(candidate, 60U));
    RUN(22U, edu21_control_validate_request(candidate), 3ULL);

    build(candidate, 3U, 0U);
    RUN(23U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 3U, 7U);
    RUN(24U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 3U, 9U);
    RUN(25U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 1U, 1U);
    RUN(26U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 2U, 32U);
    RUN(27U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 4U, 1U);
    RUN(28U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 5U, 8U);
    RUN(29U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 6U, 32U);
    RUN(30U, edu21_control_validate_request(candidate), 1ULL);

    build(candidate, 1U, 0U); put64(candidate, 16U, ~0ULL);
    put32(candidate, 60U, fnv(candidate, 60U));
    RUN(31U, edu21_control_validate_request(candidate), 0ULL);
    build(candidate, 3U, 8U); put16(candidate, 14U, 1U);
    candidate[24] ^= 1U;
    RUN(32U, edu21_control_validate_request(candidate), 1ULL);
    build(candidate, 7U, 0U); candidate[24] ^= 1U;
    RUN(33U, edu21_control_validate_request(candidate), 2ULL);

    printf(
        "OS-POST-EDU19 edu21-control-v1 snapshot=d1544b0 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
