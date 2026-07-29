typedef unsigned int u32;
typedef unsigned long long u64;

extern u64 edu29_activation_select(
    const u32 *states, const u32 *entry_valid,
    u32 slot_count, u32 runner_active);
extern int edu29_runner_identity_valid(
    u32 runner_active, u64 runner_slot, u64 runner_generation,
    u64 runner_request, u32 entry_state, u64 entry_slot,
    u64 entry_generation, u64 entry_request, u32 phase);
extern int edu30_work_shape_valid(
    u32 requested_work, u32 effective_work,
    u32 consumed_work, u32 phase);
extern u64 edu30_budget_value(
    u32 requested_work, u32 effective_work, u32 consumed_work);
extern u64 edu30_stop_action(
    u32 cancel_running, u32 requested_work, u32 effective_work,
    u32 consumed_work, u32 phase);
extern int edu30_cancel_terminal_valid(
    u32 was_running, u32 flags, u32 requested_work, u32 effective_work,
    u32 consumed_work, u32 phase, u32 reason,
    u64 terminal_value, u32 cancellation_event_seen);
extern int edu30_budget_terminal_valid(
    u32 requested_work, u32 effective_work, u32 consumed_work,
    u32 phase, u32 reason, u64 terminal_value, u32 running_published);
extern int printf(const char *format, ...);

static void clear(u32 *states, u32 *valid) {
    u32 index;
    for (index = 0; index < 8U; index = index + 1U) {
        states[index] = 0U;
        valid[index] = 0U;
    }
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
    u32 states[8];
    u32 valid[8];
    u64 value;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    clear(states, valid);
    RUN(1U, edu29_activation_select(states, valid, 8U, 0U), 8U);
    states[0] = 1U; valid[0] = 1U;
    RUN(2U, edu29_activation_select(states, valid, 8U, 0U), 0U);
    states[0] = 3U; states[1] = 1U; valid[1] = 1U;
    RUN(3U, edu29_activation_select(states, valid, 8U, 0U), 1U);
    states[1] = 4U; states[2] = 5U; valid[2] = 1U;
    states[3] = 1U; valid[3] = 1U;
    RUN(4U, edu29_activation_select(states, valid, 8U, 0U), 3U);
    RUN(5U, edu29_activation_select(states, valid, 8U, 1U), 9U);
    RUN(6U, edu29_activation_select(states, valid, 8U, 2U), 10U);
    RUN(7U, edu29_activation_select(
        (const u32 *)0, valid, 8U, 0U), 10U);
    RUN(8U, edu29_activation_select(states, valid, 9U, 0U), 10U);
    clear(states, valid); states[0] = 0U; valid[0] = 1U;
    RUN(9U, edu29_activation_select(states, valid, 8U, 0U), 10U);
    clear(states, valid); states[0] = 1U; valid[0] = 0U;
    RUN(10U, edu29_activation_select(states, valid, 8U, 0U), 10U);
    clear(states, valid); states[0] = 2U; valid[0] = 1U;
    RUN(11U, edu29_activation_select(states, valid, 8U, 0U), 10U);
    clear(states, valid); states[0] = 6U; valid[0] = 1U;
    RUN(12U, edu29_activation_select(states, valid, 8U, 0U), 10U);
    clear(states, valid); states[0] = 3U; valid[0] = 2U;
    RUN(13U, edu29_activation_select(states, valid, 8U, 0U), 10U);

    RUN(14U, edu29_runner_identity_valid(
        1U, 4ULL, 7ULL, 0x104ULL,
        2U, 4ULL, 7ULL, 0x104ULL, 0U), 1U);
    RUN(15U, edu29_runner_identity_valid(
        1U, 7ULL, 8ULL, 0x107ULL,
        2U, 7ULL, 8ULL, 0x107ULL, 4U), 1U);
    RUN(16U, edu29_runner_identity_valid(
        0U, 4ULL, 7ULL, 0x104ULL,
        2U, 4ULL, 7ULL, 0x104ULL, 0U), 0U);
    RUN(17U, edu29_runner_identity_valid(
        1U, 8ULL, 7ULL, 0x104ULL,
        2U, 8ULL, 7ULL, 0x104ULL, 0U), 0U);
    RUN(18U, edu29_runner_identity_valid(
        1U, 4ULL, 0ULL, 0x104ULL,
        2U, 4ULL, 0ULL, 0x104ULL, 0U), 0U);
    RUN(19U, edu29_runner_identity_valid(
        1U, 4ULL, 7ULL, 0x104ULL,
        3U, 4ULL, 7ULL, 0x104ULL, 0U), 0U);
    RUN(20U, edu29_runner_identity_valid(
        1U, 4ULL, 7ULL, 0x104ULL,
        2U, 5ULL, 7ULL, 0x104ULL, 0U), 0U);
    RUN(21U, edu29_runner_identity_valid(
        1U, 4ULL, 7ULL, 0x104ULL,
        2U, 4ULL, 8ULL, 0x104ULL, 0U), 0U);
    RUN(22U, edu29_runner_identity_valid(
        1U, 4ULL, 7ULL, 0x104ULL,
        2U, 4ULL, 7ULL, 0x105ULL, 0U), 0U);
    RUN(23U, edu29_runner_identity_valid(
        1U, 4ULL, 7ULL, 0x104ULL,
        2U, 4ULL, 7ULL, 0x104ULL, 5U), 0U);

    RUN(24U, edu30_work_shape_valid(1U, 1U, 0U, 0U), 1U);
    RUN(25U, edu30_work_shape_valid(4U, 4U, 4U, 4U), 1U);
    RUN(26U, edu30_work_shape_valid(0U, 0U, 0U, 0U), 0U);
    RUN(27U, edu30_work_shape_valid(5U, 5U, 0U, 0U), 0U);
    RUN(28U, edu30_work_shape_valid(2U, 1U, 0U, 0U), 0U);
    RUN(29U, edu30_work_shape_valid(2U, 2U, 1U, 0U), 0U);
    RUN(30U, edu30_work_shape_valid(4U, 4U, 5U, 5U), 0U);

    RUN(31U, edu30_budget_value(1U, 1U, 0U), 1ULL | (1ULL << 16));
    value = 4ULL | (4ULL << 16) | (4ULL << 32);
    RUN(32U, edu30_budget_value(4U, 4U, 4U), value);
    RUN(33U, edu30_budget_value(0x10000U, 1U, 0U), ~0ULL);

    RUN(34U, edu30_stop_action(0U, 4U, 4U, 0U, 0U), 3U);
    RUN(35U, edu30_stop_action(0U, 1U, 1U, 1U, 1U), 2U);
    RUN(36U, edu30_stop_action(0U, 2U, 2U, 2U, 2U), 2U);
    RUN(37U, edu30_stop_action(0U, 3U, 3U, 3U, 3U), 2U);
    RUN(38U, edu30_stop_action(0U, 4U, 4U, 4U, 4U), 3U);
    RUN(39U, edu30_stop_action(1U, 1U, 1U, 1U, 1U), 1U);
    RUN(40U, edu30_stop_action(1U, 4U, 4U, 4U, 4U), 1U);
    RUN(41U, edu30_stop_action(2U, 4U, 4U, 0U, 0U), 4U);
    RUN(42U, edu30_stop_action(0U, 4U, 3U, 0U, 0U), 4U);
    RUN(43U, edu30_stop_action(0U, 4U, 4U, 1U, 0U), 4U);

    value = 3ULL | (3ULL << 16) | (2ULL << 32);
    RUN(44U, edu30_cancel_terminal_valid(
        1U, 2U, 3U, 3U, 2U, 2U, 3U, value, 1U), 1U);
    RUN(45U, edu30_cancel_terminal_valid(
        1U, 1U, 3U, 3U, 2U, 2U, 3U, value, 1U), 0U);
    RUN(46U, edu30_cancel_terminal_valid(
        1U, 2U, 3U, 3U, 2U, 2U, 8U, value, 1U), 0U);
    RUN(47U, edu30_cancel_terminal_valid(
        1U, 2U, 3U, 3U, 2U, 2U, 3U, value + 1ULL, 1U), 0U);
    RUN(48U, edu30_cancel_terminal_valid(
        1U, 2U, 3U, 3U, 2U, 2U, 3U, value, 0U), 0U);
    value = 3ULL;
    RUN(49U, edu30_cancel_terminal_valid(
        0U, 1U, 3U, 0U, 0U, 0U, 3U, value, 1U), 1U);
    RUN(50U, edu30_cancel_terminal_valid(
        0U, 2U, 3U, 0U, 0U, 0U, 3U, value, 1U), 0U);

    value = 2ULL | (2ULL << 16) | (2ULL << 32);
    RUN(51U, edu30_budget_terminal_valid(
        2U, 2U, 2U, 2U, 8U, value, 1U), 1U);
    RUN(52U, edu30_budget_terminal_valid(
        2U, 2U, 1U, 1U, 8U, value, 1U), 0U);
    RUN(53U, edu30_budget_terminal_valid(
        4U, 4U, 4U, 4U, 8U,
        4ULL | (4ULL << 16) | (4ULL << 32), 1U), 0U);
    RUN(54U, edu30_budget_terminal_valid(
        2U, 2U, 2U, 2U, 3U, value, 1U), 0U);
    RUN(55U, edu30_budget_terminal_valid(
        2U, 2U, 2U, 2U, 8U, value, 0U), 0U);
    RUN(56U, edu30_budget_terminal_valid(
        2U, 2U, 2U, 2U, 8U, value + 1ULL, 1U), 0U);

    printf(
        "OS-POST-EDU19 edu29-30-async-stop snapshots=2b8189a+fac8fd4 "
        "vectors=%d digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
