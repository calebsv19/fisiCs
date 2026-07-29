typedef unsigned int u32;
typedef unsigned long long u64;

extern u64 edu31_calibration_window_hz(u64 tsc_delta);
extern u64 edu31_calibration_pair_hz(u64 first_hz, u64 second_hz);
extern u64 edu31_delta_to_ns(u64 delta, u64 frequency_hz);
extern u64 edu31_raw_delta(u64 current_tsc, u64 epoch_tsc);
extern int edu31_read_admissible(
    u64 last_ticks, u64 last_ns, u64 current_ticks, u64 current_ns);
extern u32 edu31_snapshot_flags(void);
extern int printf(const char *format, ...);

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
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    RUN(1U, edu31_calibration_window_hz(0ULL), 0U);
    RUN(2U, edu31_calibration_window_hz(95448ULL), 1193182U);
    RUN(3U, edu31_calibration_window_hz(190896ULL), 2386364U);
    RUN(4U, edu31_calibration_window_hz(~0ULL), 0U);

    RUN(5U, edu31_calibration_pair_hz(1000000ULL, 1000000ULL), 1000000U);
    RUN(6U, edu31_calibration_pair_hz(
        10000000000ULL, 10000000000ULL), 10000000000ULL);
    RUN(7U, edu31_calibration_pair_hz(2000000ULL, 3000000ULL), 2500000U);
    RUN(8U, edu31_calibration_pair_hz(2000000ULL, 3000001ULL), 0U);
    RUN(9U, edu31_calibration_pair_hz(3000000ULL, 2000000ULL), 2500000U);
    RUN(10U, edu31_calibration_pair_hz(999999ULL, 999999ULL), 0U);
    RUN(11U, edu31_calibration_pair_hz(
        10000000001ULL, 10000000001ULL), 0U);
    RUN(12U, edu31_calibration_pair_hz(~0ULL, ~0ULL), 0U);

    RUN(13U, edu31_delta_to_ns(0ULL, 1000000ULL), 0U);
    RUN(14U, edu31_delta_to_ns(1000000ULL, 1000000ULL), 1000000000U);
    RUN(15U, edu31_delta_to_ns(500000ULL, 1000000ULL), 500000000U);
    RUN(16U, edu31_delta_to_ns(1ULL, 1000000ULL), 1000U);
    RUN(17U, edu31_delta_to_ns(3ULL, 2000000ULL), 1500U);
    RUN(18U, edu31_delta_to_ns(
        10000000000ULL, 10000000000ULL), 1000000000U);
    RUN(19U, edu31_delta_to_ns(1ULL, 999999ULL), ~0ULL);
    RUN(20U, edu31_delta_to_ns(
        1ULL, 10000000001ULL), ~0ULL);
    RUN(21U, edu31_delta_to_ns(~0ULL, 1000000ULL), ~0ULL);
    RUN(22U, edu31_delta_to_ns(
        18446744073ULL, 1000000000ULL), 18446744073ULL);

    RUN(23U, edu31_raw_delta(10ULL, 4ULL), 6U);
    RUN(24U, edu31_raw_delta(4ULL, ~5ULL), 10U);
    RUN(25U, edu31_raw_delta(0ULL, 0ULL), 0U);
    RUN(26U, edu31_raw_delta(~0ULL, 0ULL), ~0ULL);

    RUN(27U, edu31_read_admissible(0ULL, 0ULL, 0ULL, 0ULL), 1U);
    RUN(28U, edu31_read_admissible(1ULL, 1ULL, 2ULL, 2ULL), 1U);
    RUN(29U, edu31_read_admissible(2ULL, 1ULL, 1ULL, 2ULL), 0U);
    RUN(30U, edu31_read_admissible(1ULL, 2ULL, 2ULL, 1ULL), 0U);
    RUN(31U, edu31_read_admissible(
        ~0ULL, ~0ULL, ~0ULL, ~0ULL), 1U);
    RUN(32U, edu31_snapshot_flags(), 0x00070101U);

    printf(
        "OS-POST-EDU19 edu31-time snapshot=0d10b3d vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
