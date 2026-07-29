typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

extern int edu26_queue_meta_valid(
    const u8 *p, u64 entry_lba, u32 entry_count);
extern u64 edu26_queue_entry_generation_action(
    const u8 *entry, const u8 *metadata, u64 slot);
extern int edu26_generation_reservable(u32 next_generation);
extern int edu26_ack_identity_valid(
    u32 slot, u32 state, u32 entry_generation, u64 entry_request,
    u32 acknowledged_generation, u64 acknowledged_request);
extern int printf(const char *format, ...);

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

static u32 fnv(const u8 *p, u32 count) {
    u32 hash = 2166136261U;
    u32 index;
    for (index = 0; index < count; index = index + 1U) {
        hash = (hash ^ p[index]) * 16777619U;
    }
    return hash;
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
    put32(metadata, 508U, fnv(metadata, 508U));
}

static void build_meta(u8 *metadata) {
    u32 slot;
    zero_bytes(metadata, 512U);
    put64(metadata, 0U, 0x0000513531554445ULL);
    put32(metadata, 8U, 5U);
    put32(metadata, 12U, 8U);
    put32(metadata, 16U, 306U);
    put32(metadata, 24U, 3U);
    put32(metadata, 28U, 512U);
    put32(metadata, 32U, 1U);
    put32(metadata, 36U, 32U);
    put32(metadata, 40U, 12U);
    for (slot = 0; slot < 8U; slot = slot + 1U) {
        put32(metadata, 44U + slot * 4U, 2U);
    }
    seal(metadata);
}

static void build_entry(u8 *entry, u32 generation, u64 request, u32 state) {
    zero_bytes(entry, 512U);
    put32(entry, 12U, generation);
    put64(entry, 16U, request);
    put32(entry, 24U, state);
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
    u8 entry[512];
    u8 unaligned[513];
    u32 slot;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    build_meta(metadata);
    RUN(1U, edu26_queue_meta_valid(metadata, 306ULL, 8U), 1U);
    RUN(2U, edu26_queue_meta_valid(metadata, 305ULL, 8U), 0U);
    RUN(3U, edu26_queue_meta_valid(metadata, 306ULL, 7U), 0U);
    RUN(4U, edu26_queue_meta_valid((const u8 *)0, 306ULL, 8U), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 8U, 4U); seal(candidate);
    RUN(5U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 44U, 0U); seal(candidate);
    RUN(6U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 0U);
    copy_bytes(candidate, metadata, 512U); put32(candidate, 76U, 1U); seal(candidate);
    RUN(7U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 0U);
    copy_bytes(candidate, metadata, 512U); put64(candidate, 108U, 0x101ULL); seal(candidate);
    RUN(8U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 0U);
    copy_bytes(candidate, metadata, 512U);
    put32(candidate, 76U + 4U * 4U, 1U);
    put64(candidate, 108U + 4U * 8U, 0x104ULL);
    seal(candidate);
    RUN(9U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 1U);
    copy_bytes(candidate, metadata, 512U);
    put32(candidate, 76U + 4U * 4U, 2U);
    put64(candidate, 108U + 4U * 8U, 0x104ULL);
    seal(candidate);
    RUN(10U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[172U] = 1U; seal(candidate);
    RUN(11U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 0U);
    copy_bytes(candidate, metadata, 512U); candidate[508U] ^= 1U;
    RUN(12U, edu26_queue_meta_valid(candidate, 306ULL, 8U), 0U);
    unaligned[0U] = 0xA5U; copy_bytes(unaligned + 1U, metadata, 512U);
    RUN(13U, edu26_queue_meta_valid(unaligned + 1U, 306ULL, 8U), 1U);

    build_entry(entry, 1U, 0x104ULL, 3U);
    RUN(14U, edu26_queue_entry_generation_action(entry, metadata, 4U), 0U);
    copy_bytes(candidate, metadata, 512U);
    put32(candidate, 76U + 4U * 4U, 1U);
    put64(candidate, 108U + 4U * 8U, 0x104ULL);
    RUN(15U, edu26_queue_entry_generation_action(entry, candidate, 4U), 4U);
    put32(entry, 24U, 4U);
    RUN(16U, edu26_queue_entry_generation_action(entry, candidate, 4U), 4U);
    put32(entry, 24U, 5U);
    RUN(17U, edu26_queue_entry_generation_action(entry, candidate, 4U), 4U);
    put32(entry, 24U, 2U);
    RUN(18U, edu26_queue_entry_generation_action(entry, candidate, 4U), 1U);
    put32(entry, 24U, 3U); put64(entry, 16U, 0x105ULL);
    RUN(19U, edu26_queue_entry_generation_action(entry, candidate, 4U), 0U);
    put64(entry, 16U, 0x104ULL); put32(entry, 12U, 2U);
    RUN(20U, edu26_queue_entry_generation_action(entry, candidate, 4U), 1U);
    put32(entry, 12U, 0U);
    RUN(21U, edu26_queue_entry_generation_action(entry, candidate, 4U), 1U);
    build_entry(entry, 1U, 0x104ULL, 3U);
    put32(candidate, 76U + 3U * 4U, 1U);
    put64(candidate, 108U + 3U * 8U, 0x104ULL);
    RUN(22U, edu26_queue_entry_generation_action(entry, candidate, 3U), 1U);
    RUN(23U, edu26_queue_entry_generation_action(entry, candidate, 8U), 1U);
    RUN(24U, edu26_queue_entry_generation_action(
        (const u8 *)0, candidate, 4U), 1U);

    RUN(25U, edu26_generation_reservable(0U), 0U);
    RUN(26U, edu26_generation_reservable(1U), 1U);
    RUN(27U, edu26_generation_reservable(0xfffffffeU), 1U);
    RUN(28U, edu26_generation_reservable(0xffffffffU), 0U);

    for (slot = 4U; slot < 8U; slot = slot + 1U) {
        RUN(29U + slot - 4U, edu26_ack_identity_valid(
            slot, 3U, 7U, 0x700ULL + slot,
            7U, 0x700ULL + slot), 1U);
    }
    RUN(33U, edu26_ack_identity_valid(
        3U, 3U, 7U, 0x703ULL, 7U, 0x703ULL), 0U);
    RUN(34U, edu26_ack_identity_valid(
        4U, 2U, 7U, 0x704ULL, 7U, 0x704ULL), 0U);
    RUN(35U, edu26_ack_identity_valid(
        4U, 6U, 7U, 0x704ULL, 7U, 0x704ULL), 0U);
    RUN(36U, edu26_ack_identity_valid(
        4U, 3U, 0U, 0x704ULL, 0U, 0x704ULL), 0U);
    RUN(37U, edu26_ack_identity_valid(
        4U, 3U, 7U, 0ULL, 7U, 0ULL), 0U);
    RUN(38U, edu26_ack_identity_valid(
        4U, 3U, 7U, 0x704ULL, 6U, 0x704ULL), 0U);
    RUN(39U, edu26_ack_identity_valid(
        4U, 3U, 7U, 0x704ULL, 7U, 0x705ULL), 0U);

    printf(
        "OS-POST-EDU19 edu26-generation-reuse snapshot=8359429 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
