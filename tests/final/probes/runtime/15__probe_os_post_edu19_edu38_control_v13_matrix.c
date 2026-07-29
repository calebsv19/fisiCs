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

static void copy64(u8 *destination, const u8 *source) {
    u32 index;
    for (index = 0U; index < 64U; index = index + 1U) {
        destination[index] = source[index];
    }
}

static u16 payload_for(u32 operation) {
    if (operation == 3U) return 28U;
    if (operation == 4U || operation == 6U ||
        operation == 7U || operation == 10U) return 8U;
    if (operation == 8U) return 12U;
    if (operation == 9U) return 10U;
    if (operation == 11U) return 12U;
    if (operation == 12U) return 24U;
    if (operation == 13U || operation == 16U) return 4U;
    if (operation == 15U) return 6U;
    if (operation == 18U || operation == 19U) return 12U;
    if (operation == 20U) return 8U;
    return 0U;
}

static void finish(u8 *bytes) {
    put32(bytes, 60U, fnv(bytes, 60U));
}

static void build_request(u8 *bytes, u32 operation) {
    u32 index;
    u16 payload = payload_for(operation);
    for (index = 0U; index < 64U; index = index + 1U) bytes[index] = 0U;
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '2';
    bytes[4] = '1'; bytes[5] = 'R'; bytes[6] = 'Q';
    put16(bytes, 8U, 13U);
    bytes[10] = (u8)operation;
    put16(bytes, 12U, payload);
    put64(bytes, 16U, 0xED38000000000001ULL);
    if (operation == 4U || operation == 6U || operation == 7U ||
        operation == 8U || operation == 9U || operation == 10U ||
        operation == 18U || operation == 19U || operation == 20U) {
        put64(bytes, 24U, 11ULL);
    }
    if (operation == 3U) {
        put16(bytes, 32U, 2U);
        put16(bytes, 34U, 104U);
        put32(bytes, 36U, 0xA1B2C3D4U);
        put32(bytes, 40U, 11U);
        put64(bytes, 44U, 1000ULL);
    }
    if (operation == 11U) put32(bytes, 28U, 64U);
    if (operation == 12U) {
        put32(bytes, 24U, 11U);
        put16(bytes, 28U, 0U);
        bytes[30] = 16U;
        for (index = 0U; index < 16U; index = index + 1U) {
            bytes[32U + index] = (u8)(index + 1U);
        }
    }
    if (operation == 13U || operation == 16U) put32(bytes, 24U, 11U);
    if (operation == 15U) {
        put32(bytes, 24U, 11U);
        put16(bytes, 28U, 0U);
    }
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

    for (operation = 1U; operation <= 20U; operation = operation + 1U) {
        build_request(baseline, operation);
        RUN(operation, edu21_control_validate_request(baseline), 0ULL);
    }

    build_request(candidate, 5U); put16(candidate, 8U, 12U); finish(candidate);
    RUN(21U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); put16(candidate, 8U, 14U); finish(candidate);
    RUN(22U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); candidate[10] = 0U; finish(candidate);
    RUN(23U, edu21_control_validate_request(candidate), 3ULL);
    build_request(candidate, 5U); candidate[10] = 21U; finish(candidate);
    RUN(24U, edu21_control_validate_request(candidate), 3ULL);
    build_request(candidate, 5U); put16(candidate, 12U, 33U); finish(candidate);
    RUN(25U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); put64(candidate, 16U, 0ULL); finish(candidate);
    RUN(26U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); candidate[11] = 1U; finish(candidate);
    RUN(27U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); put16(candidate, 14U, 1U); finish(candidate);
    RUN(28U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); candidate[56] = 1U; finish(candidate);
    RUN(29U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); candidate[60] ^= 1U;
    RUN(30U, edu21_control_validate_request(candidate), 2ULL);
    build_request(candidate, 5U); put16(candidate, 12U, 1U); finish(candidate);
    RUN(31U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 5U); candidate[55] = 1U; finish(candidate);
    RUN(32U, edu21_control_validate_request(candidate), 1ULL);

    build_request(candidate, 4U); put64(candidate, 24U, 0ULL); finish(candidate);
    RUN(33U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 4U);
    put64(candidate, 24U, 0x100000000ULL); finish(candidate);
    RUN(34U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U); put16(candidate, 32U, 0U); finish(candidate);
    RUN(35U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U); put16(candidate, 32U, 5U); finish(candidate);
    RUN(36U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U); put16(candidate, 34U, 103U); finish(candidate);
    RUN(37U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U); put32(candidate, 36U, 0U); finish(candidate);
    RUN(38U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U); put32(candidate, 40U, 0U); finish(candidate);
    RUN(39U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U); put64(candidate, 44U, 0ULL); finish(candidate);
    RUN(40U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U);
    put64(candidate, 44U, 60000000001ULL); finish(candidate);
    RUN(41U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 3U);
    put64(candidate, 44U, 60000000000ULL); finish(candidate);
    RUN(42U, edu21_control_validate_request(candidate), 0ULL);

    build_request(candidate, 11U); put32(candidate, 28U, 0U); finish(candidate);
    RUN(43U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 11U); put32(candidate, 28U, 2049U); finish(candidate);
    RUN(44U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 12U); put16(candidate, 28U, 128U); finish(candidate);
    RUN(45U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 12U); candidate[30] = 0U; finish(candidate);
    RUN(46U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 12U); candidate[30] = 17U; finish(candidate);
    RUN(47U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 12U); candidate[31] = 1U; finish(candidate);
    RUN(48U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 12U); candidate[47] = 0U;
    candidate[32] = 1U; candidate[33] = 0U; candidate[34] = 0U;
    candidate[35] = 0U; candidate[36] = 0U; candidate[37] = 0U;
    candidate[38] = 0U; candidate[39] = 0U; candidate[40] = 0U;
    candidate[41] = 0U; candidate[42] = 0U; candidate[43] = 0U;
    candidate[44] = 0U; candidate[45] = 0U; candidate[46] = 0U;
    candidate[47] = 1U; candidate[30] = 1U; finish(candidate);
    RUN(49U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 15U); put16(candidate, 28U, 128U); finish(candidate);
    RUN(50U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 18U); put16(candidate, 32U, 7U); finish(candidate);
    RUN(51U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 18U); put16(candidate, 34U, 1U); finish(candidate);
    RUN(52U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 19U); put16(candidate, 32U, 20U); finish(candidate);
    RUN(53U, edu21_control_validate_request(candidate), 1ULL);
    build_request(candidate, 19U); put16(candidate, 34U, 1U); finish(candidate);
    RUN(54U, edu21_control_validate_request(candidate), 1ULL);

    build_request(baseline, 5U); unaligned[0] = 0xA5U;
    copy64(unaligned + 1U, baseline);
    RUN(55U, edu21_control_validate_request(unaligned + 1U), 0ULL);
    build_request(candidate, 5U);
    put64(candidate, 16U, 0xFFFFFFFFFFFFFFFFULL); finish(candidate);
    RUN(56U, edu21_control_validate_request(candidate), 0ULL);

    printf(
        "OS-POST-EDU19 edu38-control-v13 snapshot=59d622a vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
