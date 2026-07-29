typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;

extern int edu42_dispatch_composed_valid(
    const u8 *contexts, const u8 *mailbox, u64 context, u64 slot,
    u64 entry_address, u64 queue_generation, u64 request,
    u64 work_generation);
extern int edu42_completion_composed_valid(
    const u8 *contexts, const u8 *mailbox, u64 context, u64 slot,
    u64 entry_address, u64 queue_generation, u64 request,
    u64 work_generation, u64 ap_error);
extern u64 edu42_policy_action_pair(
    const u8 *contexts, const u8 *mailbox, const u64 *slots,
    const u64 *entry_addresses, const u64 *queue_generations,
    const u64 *requests, const u32 *cancel_running,
    const u32 *time_valid, const u64 *now_ns,
    const u64 *deadline_ns, const u64 *consumed_work,
    const u64 *effective_work, const u64 *phase);
extern int edu42_retirement_composed_valid(
    const u8 *before_contexts, const u8 *after_contexts,
    const u8 *retired_mailbox, u32 retiring_context, u64 slot,
    u64 entry_address, u64 queue_generation, u64 request,
    u64 selected_phase_after, u64 ap_error);
extern int printf(const char *format, ...);

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
    u8 *contexts, u32 context, u64 slot, u64 entry,
    u64 generation, u64 request) {
    u8 *record = contexts + context * 160U;
    put64(record, 0U, 1ULL);
    put64(record, 8U, context);
    put64(record, 16U, slot);
    put64(record, 24U, entry);
    put64(record, 32U, generation);
    put64(record, 40U, request);
}

static void both_active(u8 *contexts) {
    init_contexts(contexts);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    activate(contexts, 1U, 1ULL, 0x1200ULL, 22ULL, 0x102ULL);
}

static void dispatch_mailbox(
    u8 *mailbox, u64 context, u64 slot,
    u64 queue_generation, u64 request) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 0U, 1ULL);
    put64(mailbox, 8U, 1ULL);
    put64(mailbox, 16U, 7ULL);
    put64(mailbox, 24U, 6ULL);
    put64(mailbox, 32U, context);
    put64(mailbox, 40U, slot);
    put64(mailbox, 48U, queue_generation);
    put64(mailbox, 56U, request);
    put64(mailbox, 64U, ~0ULL);
    put64(mailbox, 72U, ~0ULL);
}

static void completion_mailbox(
    u8 *mailbox, u64 context, u64 slot,
    u64 queue_generation, u64 request,
    u64 results, u64 error_generation) {
    dispatch_mailbox(mailbox, context, slot, queue_generation, request);
    put64(mailbox, 24U, 7ULL);
    put64(mailbox, 64U, context);
    put64(mailbox, 72U, slot);
    put64(mailbox, 80U, queue_generation);
    put64(mailbox, 88U, request);
    put64(mailbox, 96U, results);
    put64(mailbox, 104U, error_generation);
}

static void retired_mailbox(
    u8 *mailbox, u64 context, u64 slot,
    u64 queue_generation, u64 request,
    u64 results, u64 error_generation) {
    completion_mailbox(
        mailbox, context, slot, queue_generation, request,
        results, error_generation);
    put64(mailbox, 0U, 0ULL);
    put64(mailbox, 8U, 0ULL);
}

static void retire_context(u8 *after, u32 context) {
    u32 index;
    u8 *record = after + context * 160U;
    for (index = 0; index < 160U; index = index + 1U) record[index] = 0U;
    put64(record, 8U, context);
    put64(record, 16U, ~0ULL);
    put64(record, 152U, ~0ULL);
}

static void init_policy(
    u64 *slots, u64 *entries, u64 *generations, u64 *requests,
    u32 *cancel, u32 *time_valid, u64 *now, u64 *deadline,
    u64 *consumed, u64 *effective, u64 *phase) {
    slots[0] = 0ULL; slots[1] = 1ULL;
    entries[0] = 0x1000ULL; entries[1] = 0x1200ULL;
    generations[0] = 21ULL; generations[1] = 22ULL;
    requests[0] = 0x101ULL; requests[1] = 0x102ULL;
    cancel[0] = 0U; cancel[1] = 0U;
    time_valid[0] = 1U; time_valid[1] = 1U;
    now[0] = 10ULL; now[1] = 10ULL;
    deadline[0] = 20ULL; deadline[1] = 20ULL;
    consumed[0] = 1ULL; consumed[1] = 1ULL;
    effective[0] = 4ULL; effective[1] = 4ULL;
    phase[0] = 1ULL; phase[1] = 1ULL;
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
    u8 contexts[320];
    u8 candidate[320];
    u8 after[320];
    u8 mailbox[112];
    u8 mailbox_candidate[112];
    u8 unaligned_contexts[321];
    u8 unaligned_mailbox[113];
    u64 slots[2], entries[2], generations[2], requests[2];
    u32 cancel[2], time_valid[2];
    u64 now[2], deadline[2], consumed[2], effective[2], phase[2];
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    both_active(contexts);
    dispatch_mailbox(mailbox, 0ULL, 0ULL, 21ULL, 0x101ULL);
    RUN(1U, edu42_dispatch_composed_valid(
        contexts, mailbox, 0ULL, 0ULL, 0x1000ULL,
        21ULL, 0x101ULL, 7ULL), 1U);
    dispatch_mailbox(mailbox, 1ULL, 1ULL, 22ULL, 0x102ULL);
    RUN(2U, edu42_dispatch_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL), 1U);
    RUN(3U, edu42_dispatch_composed_valid(
        contexts, mailbox, 0ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL), 0U);
    RUN(4U, edu42_dispatch_composed_valid(
        contexts, mailbox, 1ULL, 0ULL, 0x1000ULL,
        21ULL, 0x101ULL, 7ULL), 0U);
    RUN(5U, edu42_dispatch_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1201ULL,
        22ULL, 0x102ULL, 7ULL), 0U);
    RUN(6U, edu42_dispatch_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1200ULL,
        23ULL, 0x102ULL, 7ULL), 0U);
    RUN(7U, edu42_dispatch_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x103ULL, 7ULL), 0U);
    RUN(8U, edu42_dispatch_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 8ULL), 0U);
    copy_bytes(candidate, contexts, 320U); put64(candidate + 160U, 0U, 0ULL);
    RUN(9U, edu42_dispatch_composed_valid(
        candidate, mailbox, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL), 0U);
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 8U, 0ULL);
    RUN(10U, edu42_dispatch_composed_valid(
        contexts, mailbox_candidate, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL), 0U);

    completion_mailbox(
        mailbox, 0ULL, 0ULL, 21ULL, 0x101ULL, 3ULL, 0ULL);
    RUN(11U, edu42_completion_composed_valid(
        contexts, mailbox, 0ULL, 0ULL, 0x1000ULL,
        21ULL, 0x101ULL, 7ULL, 0ULL), 1U);
    completion_mailbox(
        mailbox, 1ULL, 1ULL, 22ULL, 0x102ULL, 3ULL, 0ULL);
    RUN(12U, edu42_completion_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL, 0ULL), 1U);
    RUN(13U, edu42_completion_composed_valid(
        contexts, mailbox, 0ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL, 0ULL), 0U);
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 64U, 0ULL);
    RUN(14U, edu42_completion_composed_valid(
        contexts, mailbox_candidate, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL, 0ULL), 0U);
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 96U, 2ULL);
    RUN(15U, edu42_completion_composed_valid(
        contexts, mailbox_candidate, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL, 0ULL), 0U);
    completion_mailbox(
        mailbox, 1ULL, 1ULL, 22ULL, 0x102ULL, 2ULL, 7ULL);
    RUN(16U, edu42_completion_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL, 5ULL), 1U);
    put64(mailbox, 104U, 6ULL);
    RUN(17U, edu42_completion_composed_valid(
        contexts, mailbox, 1ULL, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 7ULL, 5ULL), 0U);

    both_active(contexts);
    dispatch_mailbox(mailbox, 0ULL, 0ULL, 21ULL, 0x101ULL);
    init_policy(
        slots, entries, generations, requests, cancel, time_valid,
        now, deadline, consumed, effective, phase);
    RUN(18U, edu42_policy_action_pair(
        contexts, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        0x404ULL);
    cancel[0] = 1U;
    RUN(19U, edu42_policy_action_pair(
        contexts, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        0x401ULL);
    cancel[0] = 0U; now[1] = 20ULL;
    RUN(20U, edu42_policy_action_pair(
        contexts, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        0x204ULL);
    cancel[1] = 1U;
    RUN(21U, edu42_policy_action_pair(
        contexts, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        0x104ULL);
    cancel[1] = 0U; now[1] = 10ULL;
    consumed[0] = 2ULL; effective[0] = 2ULL; phase[0] = 2ULL;
    RUN(22U, edu42_policy_action_pair(
        contexts, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        0x403ULL);
    time_valid[0] = 0U;
    RUN(23U, edu42_policy_action_pair(
        contexts, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        0x405ULL);
    time_valid[0] = 1U; generations[1] = 23ULL;
    RUN(24U, edu42_policy_action_pair(
        contexts, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        ~0ULL);
    generations[1] = 22ULL;
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 32U, 2ULL);
    RUN(25U, edu42_policy_action_pair(
        contexts, mailbox_candidate, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        ~0ULL);
    copy_bytes(candidate, contexts, 320U); put64(candidate, 0U, 0ULL);
    RUN(26U, edu42_policy_action_pair(
        candidate, mailbox, slots, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        ~0ULL);
    RUN(27U, edu42_policy_action_pair(
        contexts, mailbox, (const u64 *)0, entries, generations, requests,
        cancel, time_valid, now, deadline, consumed, effective, phase),
        ~0ULL);

    both_active(contexts);
    copy_bytes(after, contexts, 320U);
    retire_context(after, 0U);
    retired_mailbox(
        mailbox, 0ULL, 0ULL, 21ULL, 0x101ULL, 3ULL, 0ULL);
    RUN(28U, edu42_retirement_composed_valid(
        contexts, after, mailbox, 0U, 0ULL, 0x1000ULL,
        21ULL, 0x101ULL, 2ULL, 0ULL), 1U);
    copy_bytes(after, contexts, 320U);
    retire_context(after, 1U);
    retired_mailbox(
        mailbox, 1ULL, 1ULL, 22ULL, 0x102ULL, 3ULL, 0ULL);
    RUN(29U, edu42_retirement_composed_valid(
        contexts, after, mailbox, 1U, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 2ULL, 0ULL), 1U);
    copy_bytes(candidate, after, 320U); candidate[0] ^= 1U;
    RUN(30U, edu42_retirement_composed_valid(
        contexts, candidate, mailbox, 1U, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 2ULL, 0ULL), 0U);
    copy_bytes(candidate, after, 320U); put64(candidate + 160U, 0U, 1ULL);
    RUN(31U, edu42_retirement_composed_valid(
        contexts, candidate, mailbox, 1U, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 2ULL, 0ULL), 0U);
    RUN(32U, edu42_retirement_composed_valid(
        contexts, after, mailbox, 1U, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 1ULL, 0ULL), 0U);
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 32U, 0ULL);
    RUN(33U, edu42_retirement_composed_valid(
        contexts, after, mailbox_candidate, 1U, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 2ULL, 0ULL), 0U);
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 96U, 2ULL);
    RUN(34U, edu42_retirement_composed_valid(
        contexts, after, mailbox_candidate, 1U, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 2ULL, 0ULL), 0U);
    copy_bytes(candidate, contexts, 320U); put64(candidate, 0U, 0ULL);
    RUN(35U, edu42_retirement_composed_valid(
        candidate, after, mailbox, 1U, 1ULL, 0x1200ULL,
        22ULL, 0x102ULL, 2ULL, 0ULL), 0U);

    both_active(contexts);
    dispatch_mailbox(mailbox, 0ULL, 0ULL, 21ULL, 0x101ULL);
    copy_bytes(unaligned_contexts + 1U, contexts, 320U);
    copy_bytes(unaligned_mailbox + 1U, mailbox, 112U);
    RUN(36U, edu42_dispatch_composed_valid(
        unaligned_contexts + 1U, unaligned_mailbox + 1U,
        0ULL, 0ULL, 0x1000ULL, 21ULL, 0x101ULL, 7ULL), 1U);

    printf(
        "OS-POST-EDU19 two-owner-composition snapshots=1efb1ac+695ec66 "
        "vectors=%d digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
