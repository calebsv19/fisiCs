typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 edu33_result_payload_valid(const u8 *bytes);
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

static void finalize(u8 *bytes) {
    put32(bytes, 72U, fnv(bytes, 72U));
    put32(bytes, 508U, fnv(bytes, 508U));
}

static void build_default(u8 *bytes) {
    u32 index;
    for (index = 0U; index < 512U; index = index + 1U) bytes[index] = 0U;
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '3';
    bytes[4] = '3'; bytes[5] = 'R'; bytes[6] = '1';
    put16(bytes, 8U, 1U);
    put16(bytes, 10U, 1U);
    put32(bytes, 12U, 80U);
    put32(bytes, 16U, 3U);
    put32(bytes, 20U, 17U);
    put64(bytes, 24U, 0x1122334455667788ULL);
    put32(bytes, 32U, 9U);
    put16(bytes, 36U, 104U);
    put32(bytes, 40U, 0xA1B2C3D4U);
    put64(bytes, 48U, 0x4054800000000000ULL);
    put64(bytes, 56U, 0x403A800000000000ULL);
    put64(bytes, 64U, 0x6EC4E5DB9E1056CFULL);
    finalize(bytes);
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

    RUN(1U, edu33_result_payload_valid(baseline), 1ULL);
    RUN(2U, edu33_result_payload_valid(baseline), 1ULL);

    copy512(candidate, baseline); put32(candidate, 16U, 0U); finalize(candidate);
    RUN(3U, edu33_result_payload_valid(candidate), 1ULL);
    copy512(candidate, baseline); put32(candidate, 16U, 7U); finalize(candidate);
    RUN(4U, edu33_result_payload_valid(candidate), 1ULL);
    copy512(candidate, baseline);
    put64(candidate, 48U, 0ULL); put64(candidate, 56U, 0ULL);
    put64(candidate, 64U, 0ULL); finalize(candidate);
    RUN(5U, edu33_result_payload_valid(candidate), 1ULL);
    copy512(candidate, baseline);
    put64(candidate, 48U, 0xFFFFFFFFFFFFFFFFULL);
    put64(candidate, 56U, 0x7FF8000000000001ULL);
    put64(candidate, 64U, 0x8000000000000000ULL); finalize(candidate);
    RUN(6U, edu33_result_payload_valid(candidate), 1ULL);
    unaligned[0] = 0xA5U; copy512(unaligned + 1U, baseline);
    RUN(7U, edu33_result_payload_valid(unaligned + 1U), 1ULL);

    copy512(candidate, baseline); candidate[0] = 'X'; finalize(candidate);
    RUN(8U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[7] = 1U; finalize(candidate);
    RUN(9U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 8U, 2U); finalize(candidate);
    RUN(10U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 10U, 2U); finalize(candidate);
    RUN(11U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 12U, 79U); finalize(candidate);
    RUN(12U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 16U, 8U); finalize(candidate);
    RUN(13U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 20U, 0U); finalize(candidate);
    RUN(14U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put64(candidate, 24U, 0ULL); finalize(candidate);
    RUN(15U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 32U, 0U); finalize(candidate);
    RUN(16U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 36U, 103U); finalize(candidate);
    RUN(17U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put16(candidate, 38U, 1U); finalize(candidate);
    RUN(18U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 40U, 0U); finalize(candidate);
    RUN(19U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 44U, 1U); finalize(candidate);
    RUN(20U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); put32(candidate, 76U, 1U); finalize(candidate);
    RUN(21U, edu33_result_payload_valid(candidate), 0ULL);

    copy512(candidate, baseline); candidate[72] ^= 1U;
    put32(candidate, 508U, fnv(candidate, 508U));
    RUN(22U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[508] ^= 1U;
    RUN(23U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[80] = 1U; finalize(candidate);
    RUN(24U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[300] = 1U; finalize(candidate);
    RUN(25U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[507] = 1U; finalize(candidate);
    RUN(26U, edu33_result_payload_valid(candidate), 0ULL);

    copy512(candidate, baseline); put32(candidate, 16U, 4U);
    RUN(27U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[48] ^= 1U;
    RUN(28U, edu33_result_payload_valid(candidate), 0ULL);
    copy512(candidate, baseline); candidate[64] ^= 1U;
    RUN(29U, edu33_result_payload_valid(candidate), 0ULL);

    copy512(candidate, baseline);
    put32(candidate, 20U, 0xFFFFFFFFU);
    put64(candidate, 24U, 0xFFFFFFFFFFFFFFFFULL);
    put32(candidate, 32U, 0xFFFFFFFFU);
    put32(candidate, 40U, 0xFFFFFFFFU);
    finalize(candidate);
    RUN(30U, edu33_result_payload_valid(candidate), 1ULL);
    copy512(candidate, baseline);
    put32(candidate, 16U, 6U);
    put32(candidate, 20U, 81U);
    put64(candidate, 24U, 0x0123456789ABCDEFULL);
    put32(candidate, 32U, 27U);
    put32(candidate, 40U, 0x10203040U);
    finalize(candidate);
    RUN(31U, edu33_result_payload_valid(candidate), 1ULL);

    printf(
        "OS-POST-EDU19 result-v1 snapshot=49e4304 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
