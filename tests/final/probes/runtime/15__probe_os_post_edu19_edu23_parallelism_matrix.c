typedef unsigned long long u64;
typedef unsigned int u32;

extern u64 edu23_admission_action(
    u32 requested_workers, u32 requested_pages,
    u64 cpu_count, u64 free_pages, u32 cancel_pending);
extern u64 edu23_grant_value(u32 requested_workers, u32 effective_workers);
extern u64 edu23_compute_value(
    u64 bsp_calls, u64 ap_dispatches, u64 ap_completions);
extern int edu23_path_evidence_valid(
    u32 requested_workers, u32 effective_workers,
    u64 bsp_calls, u64 ap_dispatches, u64 ap_completions);
extern int edu23_entry_grant_valid(
    u32 state, u32 requested_workers, u32 effective_workers,
    u32 running_published, u32 resource_granted, u64 grant_value,
    u32 compute_completed, u64 compute_value);
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
    u64 grant1 = 1ULL | (1ULL << 32);
    u64 grant2 = 2ULL | (2ULL << 32);
    u64 path2 = 3ULL | (3ULL << 16) | (3ULL << 32);
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    RUN(1U, edu23_admission_action(1U, 2U, 2ULL, 2ULL, 0U), 0U);
    RUN(2U, edu23_admission_action(2U, 2U, 2ULL, 2ULL, 0U), 0U);
    RUN(3U, edu23_admission_action(0U, 2U, 2ULL, 2ULL, 0U), 2U);
    RUN(4U, edu23_admission_action(3U, 2U, 2ULL, 2ULL, 0U), 2U);
    RUN(5U, edu23_admission_action(1U, 1U, 2ULL, 2ULL, 0U), 2U);
    RUN(6U, edu23_admission_action(1U, 2U, 1ULL, 2ULL, 0U), 2U);
    RUN(7U, edu23_admission_action(1U, 2U, 2ULL, 1ULL, 0U), 2U);
    RUN(8U, edu23_admission_action(0U, 0U, 0ULL, 0ULL, 1U), 3U);
    RUN(9U, edu23_admission_action(1U, 2U, 2ULL, 2ULL, 2U), 1U);

    RUN(10U, edu23_grant_value(1U, 1U), grant1);
    RUN(11U, edu23_grant_value(2U, 2U), grant2);
    RUN(12U, edu23_grant_value(2U, 1U), 2ULL | (1ULL << 32));
    RUN(13U, edu23_compute_value(6ULL, 0ULL, 0ULL), 6ULL);
    RUN(14U, edu23_compute_value(3ULL, 3ULL, 3ULL), path2);
    RUN(15U, edu23_compute_value(0x10000ULL, 0ULL, 0ULL), ~0ULL);

    RUN(16U, edu23_path_evidence_valid(1U, 1U, 6ULL, 0ULL, 0ULL), 1U);
    RUN(17U, edu23_path_evidence_valid(2U, 2U, 3ULL, 3ULL, 3ULL), 1U);
    RUN(18U, edu23_path_evidence_valid(1U, 1U, 3ULL, 3ULL, 3ULL), 0U);
    RUN(19U, edu23_path_evidence_valid(2U, 2U, 6ULL, 0ULL, 0ULL), 0U);
    RUN(20U, edu23_path_evidence_valid(1U, 2U, 3ULL, 3ULL, 3ULL), 0U);
    RUN(21U, edu23_path_evidence_valid(2U, 1U, 6ULL, 0ULL, 0ULL), 0U);
    RUN(22U, edu23_path_evidence_valid(1U, 1U, 6ULL, 1ULL, 0ULL), 0U);
    RUN(23U, edu23_path_evidence_valid(2U, 2U, 3ULL, 3ULL, 2ULL), 0U);

    RUN(24U, edu23_entry_grant_valid(
        1U, 1U, 0U, 0U, 0U, 0ULL, 0U, 0ULL), 1U);
    RUN(25U, edu23_entry_grant_valid(
        5U, 2U, 0U, 0U, 0U, 0ULL, 0U, 0ULL), 1U);
    RUN(26U, edu23_entry_grant_valid(
        1U, 1U, 1U, 0U, 0U, 0ULL, 0U, 0ULL), 0U);
    RUN(27U, edu23_entry_grant_valid(
        2U, 1U, 1U, 1U, 1U, grant1, 0U, 0ULL), 1U);
    RUN(28U, edu23_entry_grant_valid(
        2U, 2U, 2U, 1U, 1U, grant2, 0U, 0ULL), 1U);
    RUN(29U, edu23_entry_grant_valid(
        2U, 2U, 1U, 1U, 1U, grant2, 0U, 0ULL), 0U);
    RUN(30U, edu23_entry_grant_valid(
        2U, 2U, 2U, 1U, 0U, grant2, 0U, 0ULL), 0U);
    RUN(31U, edu23_entry_grant_valid(
        2U, 2U, 2U, 1U, 1U, grant1, 0U, 0ULL), 0U);
    RUN(32U, edu23_entry_grant_valid(
        4U, 1U, 1U, 1U, 1U, grant1, 0U, 0ULL), 1U);
    RUN(33U, edu23_entry_grant_valid(
        4U, 1U, 0U, 0U, 0U, 0ULL, 0U, 0ULL), 1U);
    RUN(34U, edu23_entry_grant_valid(
        3U, 1U, 1U, 1U, 1U, grant1, 1U, 6ULL), 1U);
    RUN(35U, edu23_entry_grant_valid(
        3U, 2U, 2U, 1U, 1U, grant2, 1U, path2), 1U);
    RUN(36U, edu23_entry_grant_valid(
        3U, 1U, 1U, 0U, 1U, grant1, 1U, 6ULL), 0U);
    RUN(37U, edu23_entry_grant_valid(
        3U, 1U, 1U, 1U, 1U, grant1, 0U, 6ULL), 0U);
    RUN(38U, edu23_entry_grant_valid(
        3U, 1U, 1U, 1U, 1U, grant1, 1U, path2), 0U);
    RUN(39U, edu23_entry_grant_valid(
        3U, 2U, 2U, 1U, 1U, grant2, 1U, 6ULL), 0U);
    RUN(40U, edu23_entry_grant_valid(
        6U, 1U, 1U, 1U, 1U, grant1, 1U, 6ULL), 0U);

    printf(
        "OS-POST-EDU19 edu23-parallelism snapshot=cf375ea vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
