typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;

extern int edu40_mailbox_reset_valid(const u8 *mailbox);
extern int edu40_mailbox_idle_can_begin(const u8 *mailbox);
extern u64 edu40_mailbox_next_generation(const u8 *mailbox);
extern int edu40_mailbox_dispatch_valid(
    const u8 *mailbox, u64 work_generation, u32 mode,
    u64 selected_context, u64 selected_slot,
    u64 selected_queue_generation, u64 selected_request);
extern int edu40_mailbox_ap_accepts(
    const u8 *mailbox, u64 seen_generation, u64 work_generation, u32 mode);
extern int edu40_mailbox_completion_publishable(
    const u8 *mailbox, u64 observed_generation);
extern int edu40_mailbox_completion_valid(
    const u8 *mailbox, u64 work_generation, u32 mode, u64 ap_error,
    u64 selected_context, u64 selected_slot,
    u64 selected_queue_generation, u64 selected_request);
extern int edu40_mailbox_retired_valid(
    const u8 *mailbox, u32 mode, u64 ap_error,
    u64 selected_context, u64 selected_slot,
    u64 selected_queue_generation, u64 selected_request);
extern int edu40_mailbox_unchanged(const u8 *before, const u8 *after);
extern int printf(const char *format, ...);

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

static void copy112(u8 *destination, const u8 *source) {
    u32 index;
    for (index = 0U; index < 112U; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void build_reset(u8 *mailbox) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 32U, ~0ULL);
    put64(mailbox, 40U, ~0ULL);
    put64(mailbox, 64U, ~0ULL);
    put64(mailbox, 72U, ~0ULL);
}

static void build_phase_dispatch(u8 *mailbox, u64 generation) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 0U, 1ULL);
    put64(mailbox, 8U, 1ULL);
    put64(mailbox, 16U, generation);
    put64(mailbox, 24U, generation - 1ULL);
    put64(mailbox, 32U, 0ULL);
    put64(mailbox, 40U, 4ULL);
    put64(mailbox, 48U, 7ULL);
    put64(mailbox, 56U, 0x40000001ULL);
    put64(mailbox, 64U, ~0ULL);
    put64(mailbox, 72U, ~0ULL);
}

static void build_phase_completion(u8 *mailbox, u64 generation) {
    build_phase_dispatch(mailbox, generation);
    put64(mailbox, 24U, generation);
    put64(mailbox, 64U, 0ULL);
    put64(mailbox, 72U, 4ULL);
    put64(mailbox, 80U, 7ULL);
    put64(mailbox, 88U, 0x40000001ULL);
    put64(mailbox, 96U, 3ULL);
}

static void build_legacy_dispatch(u8 *mailbox, u64 generation) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 0U, 1ULL);
    put64(mailbox, 8U, 1ULL);
    put64(mailbox, 16U, generation);
    put64(mailbox, 24U, generation - 1ULL);
    put64(mailbox, 32U, 2ULL);
    put64(mailbox, 40U, ~0ULL);
    put64(mailbox, 56U, 0xED12000000000001ULL);
    put64(mailbox, 64U, ~0ULL);
    put64(mailbox, 72U, ~0ULL);
}

static void build_legacy_completion(u8 *mailbox, u64 generation) {
    build_legacy_dispatch(mailbox, generation);
    put64(mailbox, 24U, generation);
    put64(mailbox, 64U, 2ULL);
    put64(mailbox, 72U, ~0ULL);
    put64(mailbox, 88U, 0xED12000000000001ULL);
    put64(mailbox, 96U, 1ULL);
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
    u8 baseline[112];
    u8 candidate[112];
    u8 retained[112];
    u8 unaligned[113];
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    build_reset(baseline);
    RUN(1U, edu40_mailbox_reset_valid(baseline), 1U);
    RUN(2U, edu40_mailbox_idle_can_begin(baseline), 1U);
    RUN(3U, edu40_mailbox_next_generation(baseline), 1ULL);
    RUN(4U, edu40_mailbox_reset_valid((const u8 *)0), 0U);
    copy112(candidate, baseline); put64(candidate, 0U, 1ULL);
    RUN(5U, edu40_mailbox_idle_can_begin(candidate), 0U);
    copy112(candidate, baseline); put64(candidate, 8U, 1ULL);
    RUN(6U, edu40_mailbox_idle_can_begin(candidate), 0U);
    copy112(candidate, baseline); put64(candidate, 16U, 1ULL);
    RUN(7U, edu40_mailbox_idle_can_begin(candidate), 0U);
    copy112(candidate, baseline);
    put64(candidate, 16U, ~0ULL); put64(candidate, 24U, ~0ULL);
    RUN(8U, edu40_mailbox_idle_can_begin(candidate), 0U);
    RUN(9U, edu40_mailbox_next_generation(candidate), 0ULL);
    copy112(candidate, baseline); put64(candidate, 32U, 0ULL);
    RUN(10U, edu40_mailbox_reset_valid(candidate), 0U);

    build_phase_dispatch(baseline, 1ULL);
    RUN(11U, edu40_mailbox_dispatch_valid(
        baseline, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    RUN(12U, edu40_mailbox_idle_can_begin(baseline), 0U);
    copy112(candidate, baseline); put64(candidate, 0U, 0ULL);
    RUN(13U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 8U, 0ULL);
    RUN(14U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(15U, edu40_mailbox_dispatch_valid(
        baseline, 2ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 24U, 1ULL);
    RUN(16U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 64U, 0ULL);
    RUN(17U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 96U, 1ULL);
    RUN(18U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 104U, 1ULL);
    RUN(19U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(20U, edu40_mailbox_dispatch_valid(
        baseline, 1ULL, 0U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 32U, 2ULL);
    RUN(21U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 40U, 8ULL);
    RUN(22U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 48U, 0ULL);
    RUN(23U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 56U, 0ULL);
    RUN(24U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(25U, edu40_mailbox_dispatch_valid(
        baseline, 1ULL, 2U, 1ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);

    build_legacy_dispatch(candidate, 1ULL);
    RUN(26U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 1U, 0ULL, 0ULL, 0ULL, 0ULL), 1U);
    put64(candidate, 40U, 0ULL);
    RUN(27U, edu40_mailbox_dispatch_valid(
        candidate, 1ULL, 1U, 0ULL, 0ULL, 0ULL, 0ULL), 0U);

    build_phase_dispatch(baseline, 1ULL);
    RUN(28U, edu40_mailbox_ap_accepts(baseline, 0ULL, 1ULL, 2U), 1U);
    RUN(29U, edu40_mailbox_ap_accepts(baseline, 1ULL, 1ULL, 2U), 0U);
    RUN(30U, edu40_mailbox_ap_accepts(baseline, 0ULL, 2ULL, 2U), 0U);
    RUN(31U, edu40_mailbox_ap_accepts(baseline, ~0ULL, 1ULL, 2U), 0U);
    RUN(32U, edu40_mailbox_ap_accepts(baseline, 0ULL, 1ULL, 0U), 0U);
    copy112(candidate, baseline); put64(candidate, 0U, 0ULL);
    RUN(33U, edu40_mailbox_ap_accepts(candidate, 0ULL, 1ULL, 2U), 0U);
    copy112(candidate, baseline); put64(candidate, 32U, 2ULL);
    RUN(34U, edu40_mailbox_ap_accepts(candidate, 0ULL, 1ULL, 2U), 0U);
    build_legacy_dispatch(candidate, 1ULL);
    RUN(35U, edu40_mailbox_ap_accepts(candidate, 0ULL, 1ULL, 1U), 1U);

    build_phase_dispatch(baseline, 1ULL);
    RUN(36U, edu40_mailbox_completion_publishable(baseline, 1ULL), 1U);
    RUN(37U, edu40_mailbox_completion_publishable(baseline, 2ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 24U, 1ULL);
    RUN(38U, edu40_mailbox_completion_publishable(candidate, 1ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 24U, 2ULL);
    RUN(39U, edu40_mailbox_completion_publishable(candidate, 1ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 8U, 0ULL);
    RUN(40U, edu40_mailbox_completion_publishable(candidate, 1ULL), 0U);

    build_phase_completion(baseline, 1ULL);
    RUN(41U, edu40_mailbox_completion_valid(
        baseline, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    copy112(candidate, baseline); put64(candidate, 0U, 0ULL);
    RUN(42U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 8U, 0ULL);
    RUN(43U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(44U, edu40_mailbox_completion_valid(
        baseline, 2ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 24U, 2ULL);
    RUN(45U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 64U, 1ULL);
    RUN(46U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 72U, 5ULL);
    RUN(47U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 80U, 8ULL);
    RUN(48U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 88U, 0x40000002ULL);
    RUN(49U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(50U, edu40_mailbox_completion_valid(
        baseline, 1ULL, 2U, 0ULL, 1ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(51U, edu40_mailbox_completion_valid(
        baseline, 1ULL, 2U, 0ULL, 0ULL, 5ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(52U, edu40_mailbox_completion_valid(
        baseline, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 8ULL, 0x40000001ULL), 0U);
    RUN(53U, edu40_mailbox_completion_valid(
        baseline, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000002ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 96U, 2ULL);
    RUN(54U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 96U, 4ULL);
    RUN(55U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    copy112(candidate, baseline); put64(candidate, 104U, 1ULL);
    RUN(56U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);

    copy112(candidate, baseline);
    put64(candidate, 96U, 0ULL); put64(candidate, 104U, 1ULL);
    RUN(57U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 6ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    put64(candidate, 96U, 2ULL);
    RUN(58U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 6ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    put64(candidate, 96U, 3ULL);
    RUN(59U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 6ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    put64(candidate, 96U, 4ULL);
    RUN(60U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 6ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    put64(candidate, 96U, 2ULL); put64(candidate, 104U, 0ULL);
    RUN(61U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 6ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    put64(candidate, 104U, 2ULL);
    RUN(62U, edu40_mailbox_completion_valid(
        candidate, 1ULL, 2U, 6ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);

    build_legacy_completion(candidate, 2ULL);
    RUN(63U, edu40_mailbox_completion_valid(
        candidate, 2ULL, 1U, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL), 1U);
    put64(candidate, 96U, 0ULL);
    RUN(64U, edu40_mailbox_completion_valid(
        candidate, 2ULL, 1U, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL), 0U);
    build_legacy_completion(candidate, 2ULL); put64(candidate, 56U, 1ULL);
    put64(candidate, 88U, 1ULL);
    RUN(65U, edu40_mailbox_completion_valid(
        candidate, 2ULL, 1U, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL), 0U);

    build_phase_completion(retained, 1ULL);
    put64(retained, 0U, 0ULL); put64(retained, 8U, 0ULL);
    RUN(66U, edu40_mailbox_retired_valid(
        retained, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    RUN(67U, edu40_mailbox_completion_valid(
        retained, 1ULL, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);
    RUN(68U, edu40_mailbox_idle_can_begin(retained), 1U);
    RUN(69U, edu40_mailbox_next_generation(retained), 2ULL);
    copy112(candidate, retained); put64(candidate, 8U, 1ULL);
    RUN(70U, edu40_mailbox_retired_valid(
        candidate, 2U, 0ULL, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 0U);

    RUN(71U, edu40_mailbox_unchanged(retained, retained), 1U);
    copy112(candidate, retained); candidate[111U] ^= 1U;
    RUN(72U, edu40_mailbox_unchanged(retained, candidate), 0U);
    RUN(73U, edu40_mailbox_unchanged((const u8 *)0, retained), 0U);

    unaligned[0U] = 0xA5U; copy112(unaligned + 1U, retained);
    RUN(74U, edu40_mailbox_retired_valid(
        unaligned + 1U, 2U, 0ULL,
        0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    RUN(75U, edu40_mailbox_unchanged(retained, unaligned + 1U), 1U);
    build_phase_dispatch(candidate, ~0ULL);
    RUN(76U, edu40_mailbox_dispatch_valid(
        candidate, ~0ULL, 2U, 0ULL, 4ULL, 7ULL, 0x40000001ULL), 1U);
    build_reset(candidate);
    put64(candidate, 16U, ~0ULL - 1ULL);
    put64(candidate, 24U, ~0ULL - 1ULL);
    RUN(77U, edu40_mailbox_next_generation(candidate), ~0ULL);
    put64(candidate, 16U, ~0ULL); put64(candidate, 24U, ~0ULL);
    RUN(78U, edu40_mailbox_next_generation(candidate), 0ULL);

    printf(
        "OS-POST-EDU19 edu40-mailbox-owner snapshot=1efb1ac vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
