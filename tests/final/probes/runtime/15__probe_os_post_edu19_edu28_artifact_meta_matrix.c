typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

extern u64 edu28_artifact_meta_valid(const u8 *p);
extern u32 edu28_fnv1a32(const u8 *p, u64 count);
extern int printf(const char *format, ...);

static void put16(u8 *p, u32 offset, u16 value) {
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

static void seal(u8 *metadata) {
    put32(metadata, 508U, edu28_fnv1a32(metadata, 508ULL));
}

static void build_empty(u8 *metadata) {
    zero_bytes(metadata, 512U);
    put64(metadata, 0U, 0x00414D3832554445ULL);
    put32(metadata, 8U, 1U);
    put32(metadata, 12U, 2U);
    put16(metadata, 22U, 16U);
    put32(metadata, 28U, 0x811C9DC5U);
    seal(metadata);
}

static void build_transaction(
    u8 *metadata, u32 state, u32 generation,
    u32 length, u64 request, u16 received) {
    u16 chunks = (u16)((length + 15U) / 16U);
    u16 index;
    zero_bytes(metadata, 512U);
    put64(metadata, 0U, 0x00414D3832554445ULL);
    put32(metadata, 8U, 1U);
    put32(metadata, 12U, state);
    put32(metadata, 16U, generation);
    put16(metadata, 20U, 1U);
    put16(metadata, 22U, 16U);
    put32(metadata, 24U, length);
    put32(metadata, 28U, 0x12345678U);
    put64(metadata, 32U, request);
    put16(metadata, 40U, received);
    put16(metadata, 42U, chunks);
    for (index = 0; index < received; index = index + 1U) {
        metadata[44U + index / 8U] =
            (u8)(metadata[44U + index / 8U] | (1U << (index & 7U)));
    }
    seal(metadata);
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
    u8 metadata[512];
    u8 candidate[512];
    u8 unaligned[513];
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    build_empty(metadata);
    RUN(1U, edu28_artifact_meta_valid(metadata), 2U);
    build_transaction(metadata, 1U, 1U, 1U, 0x101ULL, 0U);
    RUN(2U, edu28_artifact_meta_valid(metadata), 1U);
    build_transaction(metadata, 1U, 2U, 17U, 0x102ULL, 1U);
    RUN(3U, edu28_artifact_meta_valid(metadata), 1U);
    build_transaction(metadata, 1U, 3U, 2048U, 0x103ULL, 127U);
    RUN(4U, edu28_artifact_meta_valid(metadata), 1U);
    build_transaction(metadata, 2U, 4U, 2048U, 0x104ULL, 128U);
    RUN(5U, edu28_artifact_meta_valid(metadata), 2U);

    copy_bytes(candidate, metadata, 512U); candidate[0U] ^= 1U; seal(candidate);
    RUN(6U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 8U, 2U); seal(candidate);
    RUN(7U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 12U, 0U); seal(candidate);
    RUN(8U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 12U, 3U); seal(candidate);
    RUN(9U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 16U, 0U); seal(candidate);
    RUN(10U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put16(candidate, 20U, 2U); seal(candidate);
    RUN(11U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put16(candidate, 22U, 8U); seal(candidate);
    RUN(12U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 24U, 0U); seal(candidate);
    RUN(13U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 24U, 2049U); seal(candidate);
    RUN(14U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put64(candidate, 32U, 0ULL); seal(candidate);
    RUN(15U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put16(candidate, 40U, 127U); seal(candidate);
    RUN(16U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put16(candidate, 42U, 127U); seal(candidate);
    RUN(17U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[59U] ^= 1U; seal(candidate);
    RUN(18U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[60U] = 1U; seal(candidate);
    RUN(19U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[508U] ^= 1U;
    RUN(20U, edu28_artifact_meta_valid(candidate), 0U);

    build_transaction(metadata, 1U, 5U, 17U, 0x105ULL, 1U);
    copy_bytes(candidate, metadata, 512U); candidate[44U] = 0U; seal(candidate);
    RUN(21U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[44U] |= 2U; seal(candidate);
    RUN(22U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[44U] |= 4U; seal(candidate);
    RUN(23U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put16(candidate, 40U, 3U); seal(candidate);
    RUN(24U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put16(candidate, 42U, 129U); seal(candidate);
    RUN(25U, edu28_artifact_meta_valid(candidate), 0U);

    build_transaction(metadata, 2U, 6U, 17U, 0x106ULL, 1U);
    RUN(26U, edu28_artifact_meta_valid(metadata), 0U);
    build_transaction(metadata, 2U, 6U, 17U, 0x106ULL, 2U);
    RUN(27U, edu28_artifact_meta_valid(metadata), 2U);

    build_empty(metadata);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 24U, 1U); seal(candidate);
    RUN(28U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 28U, 0U); seal(candidate);
    RUN(29U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put64(candidate, 32U, 1ULL); seal(candidate);
    RUN(30U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); put16(candidate, 40U, 1U); seal(candidate);
    RUN(31U, edu28_artifact_meta_valid(candidate), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[44U] = 1U; seal(candidate);
    RUN(32U, edu28_artifact_meta_valid(candidate), 0U);

    build_transaction(metadata, 1U, 7U, 16U, 0x107ULL, 1U);
    unaligned[0U] = 0xA5U; copy_bytes(unaligned + 1U, metadata, 512U);
    RUN(33U, edu28_artifact_meta_valid(unaligned + 1U), 1U);
    unaligned[1U + 508U] ^= 1U;
    RUN(34U, edu28_artifact_meta_valid(unaligned + 1U), 0U);
    RUN(35U, edu28_fnv1a32((const u8 *)"", 0ULL), 0x811C9DC5U);

    printf(
        "OS-POST-EDU19 edu28-artifact-meta snapshot=c195bf2 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
