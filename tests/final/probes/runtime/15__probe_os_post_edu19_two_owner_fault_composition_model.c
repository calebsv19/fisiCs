/*
 * Compiler-side composition probe over immutable EDU-40/41 ownership
 * contracts. The imported helpers remain the frozen contract owners; this
 * model checks that their results compose fail-closed across two active
 * runners, one singleton mailbox, policy faults, and owner-local retirement.
 *
 * This is not OS source and does not model hardware, persistence, or a
 * general scheduler.
 */
typedef unsigned char edu42_u8;
typedef unsigned int edu42_u32;
typedef unsigned long long edu42_u64;

extern int edu40_mailbox_dispatch_valid(
    const edu42_u8 *mailbox, edu42_u64 work_generation, edu42_u32 mode,
    edu42_u64 selected_context, edu42_u64 selected_slot,
    edu42_u64 selected_queue_generation, edu42_u64 selected_request);
extern int edu40_mailbox_completion_valid(
    const edu42_u8 *mailbox, edu42_u64 work_generation, edu42_u32 mode,
    edu42_u64 ap_error, edu42_u64 selected_context,
    edu42_u64 selected_slot, edu42_u64 selected_queue_generation,
    edu42_u64 selected_request);
extern int edu40_mailbox_retired_valid(
    const edu42_u8 *mailbox, edu42_u32 mode, edu42_u64 ap_error,
    edu42_u64 selected_context, edu42_u64 selected_slot,
    edu42_u64 selected_queue_generation, edu42_u64 selected_request);
extern edu42_u64 edu41_active_count(const edu42_u8 *contexts);
extern int edu41_running_owner_valid(
    const edu42_u8 *contexts, edu42_u64 slot, edu42_u64 entry_address,
    edu42_u64 generation, edu42_u64 request);
extern edu42_u64 edu41_mailbox_phase_owner(const edu42_u8 *mailbox);
extern edu42_u64 edu41_choose_context(
    const edu42_u8 *contexts, edu42_u64 next_context,
    const edu42_u8 *mailbox);
extern edu42_u64 edu41_release_phase_owner(
    edu42_u64 selected_phase_context, edu42_u64 retiring_context);
extern edu42_u64 edu41_policy_action(
    edu42_u32 cancel_running, edu42_u32 time_valid,
    edu42_u64 now_ns, edu42_u64 deadline_ns,
    edu42_u64 consumed_work, edu42_u64 effective_work,
    edu42_u64 phase);
extern int edu41_retirement_preserves_peer(
    const edu42_u8 *before, const edu42_u8 *after,
    edu42_u32 retiring_context);

enum {
    EDU42_CONTEXT_COUNT = 2,
    EDU42_MODE_PHASE_BATCH = 2,
    EDU42_CONTEXT_NONE = 2
};

int edu42_dispatch_composed_valid(
    const edu42_u8 *contexts,
    const edu42_u8 *mailbox,
    edu42_u64 context,
    edu42_u64 slot,
    edu42_u64 entry_address,
    edu42_u64 queue_generation,
    edu42_u64 request,
    edu42_u64 work_generation) {
    if (context >= EDU42_CONTEXT_COUNT ||
        edu41_active_count(contexts) != EDU42_CONTEXT_COUNT ||
        !edu41_running_owner_valid(
            contexts, slot, entry_address, queue_generation, request) ||
        edu41_mailbox_phase_owner(mailbox) != context ||
        edu41_choose_context(contexts, context ^ 1ULL, mailbox) != context) {
        return 0;
    }
    return edu40_mailbox_dispatch_valid(
        mailbox, work_generation, EDU42_MODE_PHASE_BATCH,
        context, slot, queue_generation, request);
}

int edu42_completion_composed_valid(
    const edu42_u8 *contexts,
    const edu42_u8 *mailbox,
    edu42_u64 context,
    edu42_u64 slot,
    edu42_u64 entry_address,
    edu42_u64 queue_generation,
    edu42_u64 request,
    edu42_u64 work_generation,
    edu42_u64 ap_error) {
    if (context >= EDU42_CONTEXT_COUNT ||
        edu41_active_count(contexts) != EDU42_CONTEXT_COUNT ||
        !edu41_running_owner_valid(
            contexts, slot, entry_address, queue_generation, request) ||
        edu41_mailbox_phase_owner(mailbox) != context ||
        edu41_choose_context(contexts, context ^ 1ULL, mailbox) != context) {
        return 0;
    }
    return edu40_mailbox_completion_valid(
        mailbox, work_generation, EDU42_MODE_PHASE_BATCH, ap_error,
        context, slot, queue_generation, request);
}

edu42_u64 edu42_policy_action_pair(
    const edu42_u8 *contexts,
    const edu42_u8 *mailbox,
    const edu42_u64 *slots,
    const edu42_u64 *entry_addresses,
    const edu42_u64 *queue_generations,
    const edu42_u64 *requests,
    const edu42_u32 *cancel_running,
    const edu42_u32 *time_valid,
    const edu42_u64 *now_ns,
    const edu42_u64 *deadline_ns,
    const edu42_u64 *consumed_work,
    const edu42_u64 *effective_work,
    const edu42_u64 *phase) {
    edu42_u64 mailbox_owner;
    edu42_u64 actions[2];
    edu42_u32 context;
    if (contexts == (const edu42_u8 *)0 ||
        mailbox == (const edu42_u8 *)0 ||
        slots == (const edu42_u64 *)0 ||
        entry_addresses == (const edu42_u64 *)0 ||
        queue_generations == (const edu42_u64 *)0 ||
        requests == (const edu42_u64 *)0 ||
        cancel_running == (const edu42_u32 *)0 ||
        time_valid == (const edu42_u32 *)0 ||
        now_ns == (const edu42_u64 *)0 ||
        deadline_ns == (const edu42_u64 *)0 ||
        consumed_work == (const edu42_u64 *)0 ||
        effective_work == (const edu42_u64 *)0 ||
        phase == (const edu42_u64 *)0 ||
        edu41_active_count(contexts) != EDU42_CONTEXT_COUNT) return ~0ULL;
    mailbox_owner = edu41_mailbox_phase_owner(mailbox);
    if (mailbox_owner >= EDU42_CONTEXT_COUNT) return ~0ULL;
    for (context = 0; context < EDU42_CONTEXT_COUNT;
         context = context + 1) {
        if (!edu41_running_owner_valid(
                contexts, slots[context], entry_addresses[context],
                queue_generations[context], requests[context])) return ~0ULL;
        actions[context] = edu41_policy_action(
            cancel_running[context], time_valid[context],
            now_ns[context], deadline_ns[context],
            consumed_work[context], effective_work[context],
            phase[context]);
        if (actions[context] > 5ULL) return ~0ULL;
    }
    return actions[0] | (actions[1] << 8);
}

int edu42_retirement_composed_valid(
    const edu42_u8 *before_contexts,
    const edu42_u8 *after_contexts,
    const edu42_u8 *retired_mailbox,
    edu42_u32 retiring_context,
    edu42_u64 slot,
    edu42_u64 entry_address,
    edu42_u64 queue_generation,
    edu42_u64 request,
    edu42_u64 selected_phase_after,
    edu42_u64 ap_error) {
    if (retiring_context >= EDU42_CONTEXT_COUNT ||
        edu41_active_count(before_contexts) != EDU42_CONTEXT_COUNT ||
        edu41_active_count(after_contexts) != 1ULL ||
        !edu41_running_owner_valid(
            before_contexts, slot, entry_address,
            queue_generation, request) ||
        !edu41_retirement_preserves_peer(
            before_contexts, after_contexts, retiring_context) ||
        edu41_mailbox_phase_owner(retired_mailbox) != EDU42_CONTEXT_NONE ||
        edu41_release_phase_owner(
            retiring_context, retiring_context) != selected_phase_after) {
        return 0;
    }
    return edu40_mailbox_retired_valid(
        retired_mailbox, EDU42_MODE_PHASE_BATCH, ap_error,
        retiring_context, slot, queue_generation, request);
}
