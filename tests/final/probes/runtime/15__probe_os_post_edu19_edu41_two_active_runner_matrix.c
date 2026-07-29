typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;

extern u64 edu41_active_count(const u8 *contexts);
extern int edu41_running_owner_valid(
    const u8 *contexts, u64 slot, u64 entry_address,
    u64 generation, u64 request);
extern u64 edu41_activation_select(
    const u8 *contexts, const u32 *states,
    const u64 *entry_addresses, const u64 *generations,
    const u64 *requests, u32 slot_count);
extern u64 edu41_mailbox_phase_owner(const u8 *mailbox);
extern u64 edu41_choose_context(
    const u8 *contexts, u64 next_context, const u8 *mailbox);
extern u64 edu41_next_turn(u64 selected_context);
extern u64 edu41_release_phase_owner(
    u64 selected_phase_context, u64 retiring_context);
extern u64 edu41_policy_action(
    u32 cancel_running, u32 time_valid,
    u64 now_ns, u64 deadline_ns,
    u64 consumed_work, u64 effective_work, u64 phase);
extern int edu41_retirement_preserves_peer(
    const u8 *before, const u8 *after, u32 retiring_context);
extern int printf(const char *format, ...);

enum {
    STATE_EMPTY = 0,
    STATE_PENDING = 1,
    STATE_RUNNING = 2,
    STATE_COMPLETE = 3,
    STATE_FAILED = 4,
    STATE_CANCELLED = 5,
    SELECT_NONE = 8,
    SELECT_BUSY = 9,
    SELECT_CORRUPT = 10,
    CONTEXT_NONE = 2,
    ACTION_CANCEL = 1,
    ACTION_TIMEOUT = 2,
    ACTION_BUDGET = 3,
    ACTION_WORK = 4,
    ACTION_CORRUPT = 5
};

static void put64(u8 *bytes, u32 offset, u64 value) {
    u32 index;
    for (index = 0; index < 8U; index = index + 1U) {
        bytes[offset + index] = (u8)(value >> (index * 8U));
    }
}

static void zero_bytes(u8 *bytes, u32 count) {
    u32 index;
    for (index = 0; index < count; index = index + 1U) bytes[index] = 0U;
}

static void copy_bytes(u8 *destination, const u8 *source, u32 count) {
    u32 index;
    for (index = 0; index < count; index = index + 1U) {
        destination[index] = source[index];
    }
}

static void init_contexts(u8 *contexts) {
    zero_bytes(contexts, 320U);
    put64(contexts, 8U, 0ULL);
    put64(contexts, 16U, ~0ULL);
    put64(contexts, 152U, ~0ULL);
    put64(contexts + 160U, 8U, 1ULL);
    put64(contexts + 160U, 16U, ~0ULL);
    put64(contexts + 160U, 152U, ~0ULL);
}

static void activate(
    u8 *contexts, u32 context_id, u64 slot,
    u64 entry_address, u64 generation, u64 request) {
    u8 *record = contexts + context_id * 160U;
    put64(record, 0U, 1ULL);
    put64(record, 8U, context_id);
    put64(record, 16U, slot);
    put64(record, 24U, entry_address);
    put64(record, 32U, generation);
    put64(record, 40U, request);
}

static void idle_mailbox(u8 *mailbox) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 32U, ~0ULL);
    put64(mailbox, 40U, ~0ULL);
    put64(mailbox, 64U, ~0ULL);
    put64(mailbox, 72U, ~0ULL);
}

static void pin_mailbox(u8 *mailbox, u64 context_id) {
    idle_mailbox(mailbox);
    put64(mailbox, 0U, 1ULL);
    put64(mailbox, 8U, 1ULL);
    put64(mailbox, 16U, 7ULL);
    put64(mailbox, 24U, 6ULL);
    put64(mailbox, 32U, context_id);
    put64(mailbox, 40U, 4ULL + context_id);
    put64(mailbox, 48U, 11ULL + context_id);
    put64(mailbox, 56U, 0xED41000000000001ULL + context_id);
}

static void clear_entries(
    u32 *states, u64 *entries, u64 *generations, u64 *requests) {
    u32 slot;
    for (slot = 0; slot < 8U; slot = slot + 1U) {
        states[slot] = STATE_EMPTY;
        entries[slot] = 0ULL;
        generations[slot] = 0ULL;
        requests[slot] = 0ULL;
    }
}

static void set_entry(
    u32 *states, u64 *entries, u64 *generations, u64 *requests,
    u32 slot, u32 state) {
    states[slot] = state;
    entries[slot] = 0x1000ULL + (u64)slot * 0x200ULL;
    generations[slot] = 21ULL + slot;
    requests[slot] = 0xED41000000000100ULL + slot;
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
    u8 after[320];
    u8 mailbox[112];
    u8 unaligned_contexts[321];
    u8 unaligned_mailbox[113];
    u32 states[8];
    u64 entries[8];
    u64 generations[8];
    u64 requests[8];
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    init_contexts(contexts);
    RUN(1U, edu41_active_count(contexts), 0ULL);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    RUN(2U, edu41_active_count(contexts), 1ULL);
    activate(contexts, 1U, 1ULL, 0x1200ULL, 22ULL, 0x102ULL);
    RUN(3U, edu41_active_count(contexts), 2ULL);
    copy_bytes(candidate, contexts, 320U); put64(candidate, 0U, 2ULL);
    RUN(4U, edu41_active_count(candidate), ~0ULL);
    copy_bytes(candidate, contexts, 320U); put64(candidate + 160U, 8U, 0ULL);
    RUN(5U, edu41_active_count(candidate), ~0ULL);
    RUN(6U, edu41_active_count((const u8 *)0), ~0ULL);

    RUN(7U, edu41_running_owner_valid(
        contexts, 0ULL, 0x1000ULL, 21ULL, 0x101ULL), 1U);
    RUN(8U, edu41_running_owner_valid(
        contexts, 1ULL, 0x1200ULL, 22ULL, 0x102ULL), 1U);
    RUN(9U, edu41_running_owner_valid(
        contexts, 2ULL, 0x1000ULL, 21ULL, 0x101ULL), 0U);
    RUN(10U, edu41_running_owner_valid(
        contexts, 0ULL, 0x1001ULL, 21ULL, 0x101ULL), 0U);
    RUN(11U, edu41_running_owner_valid(
        contexts, 0ULL, 0x1000ULL, 22ULL, 0x101ULL), 0U);
    RUN(12U, edu41_running_owner_valid(
        contexts, 0ULL, 0x1000ULL, 21ULL, 0x102ULL), 0U);
    copy_bytes(candidate, contexts, 320U); put64(candidate, 0U, 0ULL);
    RUN(13U, edu41_running_owner_valid(
        candidate, 0ULL, 0x1000ULL, 21ULL, 0x101ULL), 0U);
    RUN(14U, edu41_running_owner_valid(
        contexts, 8ULL, 0x1000ULL, 21ULL, 0x101ULL), 0U);
    RUN(15U, edu41_running_owner_valid(
        contexts, 0ULL, 0ULL, 21ULL, 0x101ULL), 0U);

    init_contexts(contexts); clear_entries(states, entries, generations, requests);
    RUN(16U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_NONE);
    set_entry(states, entries, generations, requests, 0U, STATE_PENDING);
    RUN(17U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), 0U);
    clear_entries(states, entries, generations, requests);
    set_entry(states, entries, generations, requests, 1U, STATE_PENDING);
    RUN(18U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), 1U);

    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    clear_entries(states, entries, generations, requests);
    set_entry(states, entries, generations, requests, 0U, STATE_RUNNING);
    entries[0] = 0x1000ULL; generations[0] = 21ULL; requests[0] = 0x101ULL;
    set_entry(states, entries, generations, requests, 2U, STATE_PENDING);
    set_entry(states, entries, generations, requests, 3U, STATE_PENDING);
    RUN(19U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), 3U);
    states[3] = STATE_EMPTY; entries[3] = 0ULL;
    generations[3] = 0ULL; requests[3] = 0ULL;
    RUN(20U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_BUSY);
    entries[0] = 0x1001ULL;
    RUN(21U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_CORRUPT);
    entries[0] = 0x1000ULL; generations[0] = 22ULL;
    RUN(22U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_CORRUPT);
    generations[0] = 21ULL; requests[0] = 0x102ULL;
    RUN(23U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_CORRUPT);

    init_contexts(contexts);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    activate(contexts, 1U, 1ULL, 0x1200ULL, 22ULL, 0x102ULL);
    clear_entries(states, entries, generations, requests);
    set_entry(states, entries, generations, requests, 2U, STATE_PENDING);
    RUN(24U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_BUSY);
    init_contexts(contexts); clear_entries(states, entries, generations, requests);
    set_entry(states, entries, generations, requests, 0U, STATE_COMPLETE);
    set_entry(states, entries, generations, requests, 1U, STATE_FAILED);
    set_entry(states, entries, generations, requests, 2U, STATE_CANCELLED);
    RUN(25U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_NONE);
    states[3] = 6U;
    RUN(26U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_CORRUPT);
    states[3] = STATE_EMPTY; entries[3] = 1ULL;
    RUN(27U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_CORRUPT);
    entries[3] = 0ULL; states[4] = STATE_PENDING;
    RUN(28U, edu41_activation_select(
        contexts, states, entries, generations, requests, 8U), SELECT_CORRUPT);
    RUN(29U, edu41_activation_select(
        contexts, states, entries, generations, requests, 9U), SELECT_CORRUPT);
    RUN(30U, edu41_activation_select(
        contexts, (const u32 *)0, entries, generations, requests, 8U),
        SELECT_CORRUPT);

    idle_mailbox(mailbox);
    RUN(31U, edu41_mailbox_phase_owner(mailbox), CONTEXT_NONE);
    pin_mailbox(mailbox, 0ULL);
    RUN(32U, edu41_mailbox_phase_owner(mailbox), 0U);
    pin_mailbox(mailbox, 1ULL);
    RUN(33U, edu41_mailbox_phase_owner(mailbox), 1U);
    pin_mailbox(mailbox, 2ULL);
    RUN(34U, edu41_mailbox_phase_owner(mailbox), ~0ULL);
    idle_mailbox(mailbox); put64(mailbox, 0U, 1ULL);
    RUN(35U, edu41_mailbox_phase_owner(mailbox), ~0ULL);
    idle_mailbox(mailbox); put64(mailbox, 0U, 2ULL); put64(mailbox, 8U, 2ULL);
    RUN(36U, edu41_mailbox_phase_owner(mailbox), ~0ULL);
    RUN(37U, edu41_mailbox_phase_owner((const u8 *)0), ~0ULL);

    init_contexts(contexts); idle_mailbox(mailbox);
    RUN(38U, edu41_choose_context(contexts, 0ULL, mailbox), CONTEXT_NONE);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    RUN(39U, edu41_choose_context(contexts, 0ULL, mailbox), 0U);
    RUN(40U, edu41_choose_context(contexts, 1ULL, mailbox), 0U);
    init_contexts(contexts);
    activate(contexts, 1U, 1ULL, 0x1200ULL, 22ULL, 0x102ULL);
    RUN(41U, edu41_choose_context(contexts, 0ULL, mailbox), 1U);
    RUN(42U, edu41_choose_context(contexts, 1ULL, mailbox), 1U);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    RUN(43U, edu41_choose_context(contexts, 0ULL, mailbox), 0U);
    RUN(44U, edu41_choose_context(contexts, 1ULL, mailbox), 1U);
    RUN(45U, edu41_choose_context(contexts, 2ULL, mailbox), ~0ULL);
    pin_mailbox(mailbox, 0ULL);
    RUN(46U, edu41_choose_context(contexts, 1ULL, mailbox), 0U);
    pin_mailbox(mailbox, 1ULL);
    RUN(47U, edu41_choose_context(contexts, 0ULL, mailbox), 1U);
    put64(contexts + 160U, 0U, 0ULL);
    RUN(48U, edu41_choose_context(contexts, 0ULL, mailbox), ~0ULL);
    put64(contexts + 160U, 0U, 1ULL); put64(contexts, 0U, 2ULL);
    RUN(49U, edu41_choose_context(contexts, 0ULL, mailbox), ~0ULL);
    init_contexts(contexts); activate(
        contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    idle_mailbox(mailbox); put64(mailbox, 8U, 1ULL);
    RUN(50U, edu41_choose_context(contexts, 0ULL, mailbox), ~0ULL);

    RUN(51U, edu41_next_turn(0ULL), 1U);
    RUN(52U, edu41_next_turn(1ULL), 0U);
    RUN(53U, edu41_next_turn(2ULL), ~0ULL);
    RUN(54U, edu41_release_phase_owner(0ULL, 0ULL), CONTEXT_NONE);
    RUN(55U, edu41_release_phase_owner(1ULL, 0ULL), 1U);
    RUN(56U, edu41_release_phase_owner(CONTEXT_NONE, 1ULL), CONTEXT_NONE);
    RUN(57U, edu41_release_phase_owner(3ULL, 0ULL), ~0ULL);
    RUN(58U, edu41_release_phase_owner(0ULL, 2ULL), ~0ULL);

    RUN(59U, edu41_policy_action(
        1U, 1U, 200ULL, 100ULL, 4ULL, 0ULL, 4ULL), ACTION_CANCEL);
    RUN(60U, edu41_policy_action(
        0U, 0U, 0ULL, 100ULL, 0ULL, 4ULL, 0ULL), ACTION_CORRUPT);
    RUN(61U, edu41_policy_action(
        0U, 1U, 100ULL, 100ULL, 0ULL, 4ULL, 0ULL), ACTION_TIMEOUT);
    RUN(62U, edu41_policy_action(
        0U, 1U, 101ULL, 100ULL, 0ULL, 4ULL, 0ULL), ACTION_TIMEOUT);
    RUN(63U, edu41_policy_action(
        0U, 1U, 99ULL, 100ULL, 2ULL, 2ULL, 2ULL), ACTION_BUDGET);
    RUN(64U, edu41_policy_action(
        0U, 1U, 99ULL, 100ULL, 1ULL, 2ULL, 1ULL), ACTION_WORK);
    RUN(65U, edu41_policy_action(
        0U, 1U, 99ULL, 100ULL, 4ULL, 4ULL, 4ULL), ACTION_WORK);
    RUN(66U, edu41_policy_action(
        2U, 1U, 99ULL, 100ULL, 0ULL, 4ULL, 0ULL), ACTION_CORRUPT);
    RUN(67U, edu41_policy_action(
        0U, 2U, 99ULL, 100ULL, 0ULL, 4ULL, 0ULL), ACTION_CORRUPT);
    RUN(68U, edu41_policy_action(
        0U, 1U, 99ULL, 100ULL, 1ULL, 4ULL, 0ULL), ACTION_CORRUPT);
    RUN(69U, edu41_policy_action(
        0U, 1U, 99ULL, 100ULL, 5ULL, 5ULL, 5ULL), ACTION_CORRUPT);

    init_contexts(contexts);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    activate(contexts, 1U, 1ULL, 0x1200ULL, 22ULL, 0x102ULL);
    copy_bytes(after, contexts, 320U); put64(after, 0U, 0ULL);
    RUN(70U, edu41_retirement_preserves_peer(contexts, after, 0U), 1U);
    after[160U + 40U] ^= 1U;
    RUN(71U, edu41_retirement_preserves_peer(contexts, after, 0U), 0U);
    copy_bytes(after, contexts, 320U); put64(after + 160U, 0U, 0ULL);
    RUN(72U, edu41_retirement_preserves_peer(contexts, after, 1U), 1U);
    put64(after + 160U, 8U, 0ULL);
    RUN(73U, edu41_retirement_preserves_peer(contexts, after, 1U), 0U);
    RUN(74U, edu41_retirement_preserves_peer(
        contexts, after, 2U), 0U);
    RUN(75U, edu41_retirement_preserves_peer(
        (const u8 *)0, after, 0U), 0U);

    init_contexts(contexts);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    activate(contexts, 1U, 1ULL, 0x1200ULL, 22ULL, 0x102ULL);
    unaligned_contexts[0] = 0xA5U;
    copy_bytes(unaligned_contexts + 1U, contexts, 320U);
    idle_mailbox(mailbox);
    unaligned_mailbox[0] = 0x5AU;
    copy_bytes(unaligned_mailbox + 1U, mailbox, 112U);
    RUN(76U, edu41_active_count(unaligned_contexts + 1U), 2U);
    RUN(77U, edu41_choose_context(
        unaligned_contexts + 1U, 0ULL, unaligned_mailbox + 1U), 0U);
    pin_mailbox(mailbox, 1ULL);
    copy_bytes(unaligned_mailbox + 1U, mailbox, 112U);
    RUN(78U, edu41_choose_context(
        unaligned_contexts + 1U, 0ULL, unaligned_mailbox + 1U), 1U);
    RUN(79U, edu41_running_owner_valid(
        unaligned_contexts + 1U,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL), 1U);

    printf(
        "OS-POST-EDU19 edu41-two-active snapshot=695ec66 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
