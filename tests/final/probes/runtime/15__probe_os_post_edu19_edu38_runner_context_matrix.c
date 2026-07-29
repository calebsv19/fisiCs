typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 edu38_runner_context_for_slot(u64 slot);
extern u64 edu38_runner_find_active(const u8 *contexts);
extern int edu38_runner_contexts_valid(const u8 *contexts);
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

static void zero_bytes(u8 *bytes, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) bytes[index] = 0U;
}

static void copy320(u8 *destination, const u8 *source) {
    u32 index;
    for (index = 0U; index < 320U; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void init_contexts(u8 *contexts) {
    zero_bytes(contexts, 320U);
    put64(contexts, 8U, 0ULL);
    put64(contexts, 16U, 0xFFFFFFFFFFFFFFFFULL);
    put64(contexts, 152U, 0xFFFFFFFFFFFFFFFFULL);
    put64(contexts + 160U, 8U, 1ULL);
    put64(contexts + 160U, 16U, 0xFFFFFFFFFFFFFFFFULL);
    put64(contexts + 160U, 152U, 0xFFFFFFFFFFFFFFFFULL);
}

static void activate(u8 *contexts, u32 lane, u64 slot) {
    u8 *record = contexts + lane * 160U;
    put64(record, 0U, 1ULL);
    put64(record, 8U, lane);
    put64(record, 16U, slot);
    put64(record, 24U, 0x1000ULL + lane * 0x100ULL);
    put64(record, 32U, 11ULL + lane);
    put64(record, 40U, 0xED38000000000001ULL + lane);
    put64(record, 48U, 1000ULL);
    put32(record, 56U, 11U);
    put16(record, 60U, 104U);
    put16(record, 62U, 0U);
    put64(record, 64U, 0xA1B2C3D4ULL);
    put64(record, 72U, 0x2000ULL + lane);
    put64(record, 80U, 0x3000ULL + lane * 0x100ULL);
    put64(record, 88U, 0x4000ULL + lane);
    put64(record, 96U, 3ULL);
    put64(record, 104U, 1ULL);
    put64(record, 144U, 3ULL);
    put64(record, 152U, lane);
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
    u8 contexts[320];
    u8 candidate[320];
    u8 unaligned[321];
    u32 slot;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    init_contexts(contexts);
    RUN(1U, edu38_runner_contexts_valid(contexts), 1U);
    RUN(2U, edu38_runner_find_active(contexts), 2U);
    for (slot = 0U; slot < 10U; slot = slot + 1U) {
        RUN(3U + slot, edu38_runner_context_for_slot(slot), slot & 1U);
    }
    RUN(13U, edu38_runner_context_for_slot(0xFFFFFFFFFFFFFFFFULL), 1U);

    init_contexts(contexts); activate(contexts, 0U, 0U);
    RUN(14U, edu38_runner_contexts_valid(contexts), 1U);
    RUN(15U, edu38_runner_find_active(contexts), 0U);
    init_contexts(contexts); activate(contexts, 1U, 1U);
    RUN(16U, edu38_runner_contexts_valid(contexts), 1U);
    RUN(17U, edu38_runner_find_active(contexts), 1U);
    activate(contexts, 0U, 0U);
    RUN(18U, edu38_runner_contexts_valid(contexts), 0U);
    RUN(19U, edu38_runner_find_active(contexts), 0xFFFFFFFFFFFFFFFFULL);

    init_contexts(candidate); put64(candidate, 0U, 2ULL);
    RUN(20U, edu38_runner_contexts_valid(candidate), 0U);
    RUN(21U, edu38_runner_find_active(candidate), 0U);
    init_contexts(candidate); put64(candidate + 160U, 8U, 0ULL);
    RUN(22U, edu38_runner_contexts_valid(candidate), 0U);
    init_contexts(candidate); put64(candidate, 8U, 1ULL);
    RUN(23U, edu38_runner_contexts_valid(candidate), 0U);
    init_contexts(candidate); put16(candidate, 62U, 1U);
    RUN(24U, edu38_runner_contexts_valid(candidate), 0U);
    init_contexts(candidate); put64(candidate, 152U, 0ULL);
    RUN(25U, edu38_runner_contexts_valid(candidate), 1U);
    init_contexts(candidate); put64(candidate, 152U, 1ULL);
    RUN(26U, edu38_runner_contexts_valid(candidate), 1U);
    init_contexts(candidate); put64(candidate, 152U, 2ULL);
    RUN(27U, edu38_runner_contexts_valid(candidate), 0U);

    init_contexts(contexts); activate(contexts, 0U, 0U);
    copy320(candidate, contexts); put64(candidate, 16U, 8ULL);
    RUN(28U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 24U, 0ULL);
    RUN(29U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 32U, 0ULL);
    RUN(30U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 40U, 0ULL);
    RUN(31U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 48U, 0ULL);
    RUN(32U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put32(candidate, 56U, 0U);
    RUN(33U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put16(candidate, 60U, 103U);
    RUN(34U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 64U, 0ULL);
    RUN(35U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 72U, 0ULL);
    RUN(36U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 80U, 0ULL);
    RUN(37U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 88U, 0ULL);
    RUN(38U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 96U, 6ULL);
    RUN(39U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 104U, 2ULL);
    RUN(40U, edu38_runner_contexts_valid(candidate), 0U);
    copy320(candidate, contexts); put64(candidate, 144U, 6ULL);
    RUN(41U, edu38_runner_contexts_valid(candidate), 0U);

    init_contexts(candidate);
    put64(candidate, 40U, 0xDEADBEEFULL);
    put64(candidate, 96U, 0xFFFFFFFFFFFFFFFFULL);
    RUN(42U, edu38_runner_contexts_valid(candidate), 1U);
    init_contexts(contexts); activate(contexts, 1U, 7U);
    put64(contexts + 160U, 96U, 5ULL);
    put64(contexts + 160U, 104U, 0ULL);
    put64(contexts + 160U, 144U, 5ULL);
    put64(contexts + 160U, 152U, 0xFFFFFFFFFFFFFFFFULL);
    RUN(43U, edu38_runner_contexts_valid(contexts), 1U);
    unaligned[0] = 0xA5U; copy320(unaligned + 1U, contexts);
    RUN(44U, edu38_runner_contexts_valid(unaligned + 1U), 1U);
    RUN(45U, edu38_runner_find_active(unaligned + 1U), 1U);

    printf(
        "OS-POST-EDU19 edu38-runner-context snapshot=59d622a vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
