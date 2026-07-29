/*
 * Hardware-blind compiler probe over the frozen EDU-26/35/37/39/40/41
 * ownership contracts and their EDU-43 durable-chain composition.
 *
 * This is a temporal fault-sequence model, not immutable OS source and not an
 * EDU-44 operating-system claim. It proves that observations from an earlier
 * lifecycle point cannot be re-admitted after interruption, restart, owner
 * loss, or retirement, while the unaffected peer remains valid.
 */
typedef unsigned char edu44_u8;
typedef unsigned int edu44_u32;
typedef unsigned long long edu44_u64;

extern int edu26_ack_identity_valid(
    edu44_u32 slot, edu44_u32 state, edu44_u32 entry_generation,
    edu44_u64 entry_request, edu44_u32 acknowledged_generation,
    edu44_u64 acknowledged_request);
extern int edu43_generation_reuse_owner_valid(
    const edu44_u8 *before_contexts, const edu44_u8 *after_contexts,
    edu44_u32 retiring_context, edu44_u64 reclaimed_slot,
    edu44_u64 reclaimed_entry_address, edu44_u32 reclaimed_state,
    edu44_u32 reclaimed_generation, edu44_u64 reclaimed_request,
    edu44_u32 acknowledged_generation, edu44_u64 acknowledged_request,
    edu44_u32 next_generation, edu44_u64 new_request,
    edu44_u64 peer_slot, edu44_u64 peer_entry_address,
    edu44_u64 peer_generation, edu44_u64 peer_request);
extern int edu43_checkpoint_owner_valid(
    const edu44_u8 *lanes, const edu44_u8 *entries,
    const edu44_u8 *contexts, edu44_u32 context, edu44_u32 slot,
    edu44_u64 entry_address, edu44_u32 generation,
    edu44_u64 request, edu44_u32 expected_lane);
extern int edu43_phase_owner_active_valid(
    const edu44_u8 *owners, const edu44_u8 *contexts,
    const edu44_u8 *mailbox, edu44_u32 context, edu44_u32 slot,
    edu44_u64 entry_address, edu44_u32 queue_generation,
    edu44_u64 request, edu44_u32 workload_generation,
    edu44_u32 workload_checksum, edu44_u32 width,
    const edu44_u8 *workload);
extern int edu39_phase_owner_pair_valid(const edu44_u8 *owners);
extern int edu39_phase_owner_matches(
    const edu44_u8 *record, edu44_u32 context_id,
    edu44_u32 slot, edu44_u32 queue_generation,
    edu44_u64 request_id, edu44_u32 workload_generation,
    edu44_u32 workload_length, edu44_u32 workload_checksum,
    edu44_u32 width, const edu44_u8 *workload);
extern int edu40_mailbox_completion_valid(
    const edu44_u8 *mailbox, edu44_u64 work_generation,
    edu44_u32 mode, edu44_u64 ap_error,
    edu44_u64 selected_context, edu44_u64 selected_slot,
    edu44_u64 selected_queue_generation, edu44_u64 selected_request);
extern int edu40_mailbox_dispatch_valid(
    const edu44_u8 *mailbox, edu44_u64 work_generation,
    edu44_u32 mode, edu44_u64 selected_context,
    edu44_u64 selected_slot, edu44_u64 selected_queue_generation,
    edu44_u64 selected_request);
extern int edu40_mailbox_retired_valid(
    const edu44_u8 *mailbox, edu44_u32 mode, edu44_u64 ap_error,
    edu44_u64 selected_context, edu44_u64 selected_slot,
    edu44_u64 selected_queue_generation, edu44_u64 selected_request);
extern edu44_u64 edu41_active_count(const edu44_u8 *contexts);
extern edu44_u64 edu41_mailbox_phase_owner(const edu44_u8 *mailbox);
extern int edu41_retirement_preserves_peer(
    const edu44_u8 *before, const edu44_u8 *after,
    edu44_u32 retiring_context);
extern int edu41_running_owner_valid(
    const edu44_u8 *contexts, edu44_u64 slot,
    edu44_u64 entry_address, edu44_u64 generation,
    edu44_u64 request);

enum {
    EDU44_CONTEXT_COUNT = 2,
    EDU44_CONTEXT_NONE = 2,
    EDU44_PHASE_OWNER_BYTES = 224,
    EDU44_WORKLOAD_BYTES = 104,
    EDU44_MODE_PHASE_BATCH = 2
};

static edu44_u32 edu44_read32(const edu44_u8 *bytes) {
    return (edu44_u32)bytes[0] |
           ((edu44_u32)bytes[1] << 8U) |
           ((edu44_u32)bytes[2] << 16U) |
           ((edu44_u32)bytes[3] << 24U);
}

static edu44_u64 edu44_read64(const edu44_u8 *bytes) {
    return (edu44_u64)edu44_read32(bytes) |
           ((edu44_u64)edu44_read32(bytes + 4) << 32U);
}

static int edu44_checkpoint_record_matches(
    const edu44_u8 *lanes,
    edu44_u32 lane,
    edu44_u32 slot,
    edu44_u32 generation,
    edu44_u64 request) {
    const edu44_u8 *record;
    if (lanes == (const edu44_u8 *)0 || lane >= 2U) return 0;
    record = lanes + lane * 512U;
    return edu44_read32(record + 16U) == slot &&
           edu44_read32(record + 20U) == generation &&
           edu44_read64(record + 24U) == request;
}

int edu44_pre_ack_interruption_blocks_reuse(
    const edu44_u8 *before_contexts,
    const edu44_u8 *after_contexts,
    edu44_u32 retiring_context,
    edu44_u64 reclaimed_slot,
    edu44_u64 reclaimed_entry_address,
    edu44_u32 reclaimed_state,
    edu44_u32 reclaimed_generation,
    edu44_u64 reclaimed_request,
    edu44_u32 observed_ack_generation,
    edu44_u64 observed_ack_request,
    edu44_u32 next_generation,
    edu44_u64 new_request,
    edu44_u64 peer_slot,
    edu44_u64 peer_entry_address,
    edu44_u64 peer_generation,
    edu44_u64 peer_request) {
    if (before_contexts == (const edu44_u8 *)0 ||
        after_contexts == (const edu44_u8 *)0 ||
        retiring_context >= EDU44_CONTEXT_COUNT ||
        edu41_active_count(before_contexts) != EDU44_CONTEXT_COUNT ||
        edu41_active_count(after_contexts) != 1ULL ||
        edu41_running_owner_valid(
            after_contexts, reclaimed_slot, reclaimed_entry_address,
            reclaimed_generation, reclaimed_request) ||
        !edu41_running_owner_valid(
            after_contexts, peer_slot, peer_entry_address,
            peer_generation, peer_request) ||
        !edu41_retirement_preserves_peer(
            before_contexts, after_contexts, retiring_context)) {
        return 0;
    }
    if (edu26_ack_identity_valid(
            (edu44_u32)reclaimed_slot, reclaimed_state,
            reclaimed_generation, reclaimed_request,
            observed_ack_generation, observed_ack_request)) {
        return 0;
    }
    return !edu43_generation_reuse_owner_valid(
        before_contexts, after_contexts, retiring_context,
        reclaimed_slot, reclaimed_entry_address, reclaimed_state,
        reclaimed_generation, reclaimed_request,
        observed_ack_generation, observed_ack_request,
        next_generation, new_request, peer_slot, peer_entry_address,
        peer_generation, peer_request);
}

int edu44_post_checkpoint_restart_rejects_stale(
    const edu44_u8 *old_lanes,
    const edu44_u8 *new_lanes,
    const edu44_u8 *entries,
    const edu44_u8 *before_contexts,
    const edu44_u8 *restarted_contexts,
    edu44_u32 context,
    edu44_u32 old_slot,
    edu44_u64 old_entry_address,
    edu44_u32 old_generation,
    edu44_u64 old_request,
    edu44_u32 old_lane,
    edu44_u32 new_slot,
    edu44_u64 new_entry_address,
    edu44_u32 new_generation,
    edu44_u64 new_request,
    edu44_u32 new_lane,
    edu44_u64 peer_slot,
    edu44_u64 peer_entry_address,
    edu44_u64 peer_generation,
    edu44_u64 peer_request) {
    if (old_lanes == (const edu44_u8 *)0 ||
        new_lanes == (const edu44_u8 *)0 ||
        entries == (const edu44_u8 *)0 ||
        before_contexts == (const edu44_u8 *)0 ||
        restarted_contexts == (const edu44_u8 *)0 ||
        context >= EDU44_CONTEXT_COUNT ||
        old_generation == new_generation ||
        old_request == new_request) {
        return 0;
    }
    /*
     * Lane selection alone may name a reclaimable overwrite destination.
     * A restart may count the lane as restored evidence only when the record
     * itself carries the exact new owner identity.
     */
    return edu44_checkpoint_record_matches(
               old_lanes, old_lane, old_slot,
               old_generation, old_request) &&
           !edu44_checkpoint_record_matches(
               old_lanes, old_lane, new_slot,
               new_generation, new_request) &&
           edu44_checkpoint_record_matches(
               new_lanes, new_lane, new_slot,
               new_generation, new_request) &&
           edu43_checkpoint_owner_valid(
               old_lanes, entries, before_contexts, context,
               old_slot, old_entry_address, old_generation,
               old_request, old_lane) &&
           edu43_checkpoint_owner_valid(
               new_lanes, entries, restarted_contexts, context,
               new_slot, new_entry_address, new_generation,
               new_request, new_lane) &&
           edu41_running_owner_valid(
               restarted_contexts, peer_slot, peer_entry_address,
               peer_generation, peer_request);
}

int edu44_mid_phase_owner_loss_preserves_peer(
    const edu44_u8 *owners,
    const edu44_u8 *before_contexts,
    const edu44_u8 *after_contexts,
    const edu44_u8 *lost_mailbox,
    const edu44_u8 *peer_mailbox,
    edu44_u32 lost_context,
    edu44_u32 lost_slot,
    edu44_u64 lost_entry_address,
    edu44_u32 lost_generation,
    edu44_u64 lost_request,
    edu44_u32 lost_workload_generation,
    edu44_u32 lost_width,
    edu44_u32 peer_slot,
    edu44_u64 peer_entry_address,
    edu44_u32 peer_generation,
    edu44_u64 peer_request,
    edu44_u32 peer_workload_generation,
    edu44_u32 peer_width,
    edu44_u32 workload_checksum,
    const edu44_u8 *workload) {
    edu44_u32 peer_context;
    const edu44_u8 *peer_owner;
    if (owners == (const edu44_u8 *)0 ||
        before_contexts == (const edu44_u8 *)0 ||
        after_contexts == (const edu44_u8 *)0 ||
        lost_mailbox == (const edu44_u8 *)0 ||
        peer_mailbox == (const edu44_u8 *)0 ||
        workload == (const edu44_u8 *)0 ||
        lost_context >= EDU44_CONTEXT_COUNT) {
        return 0;
    }
    peer_context = lost_context ^ 1U;
    peer_owner = owners + peer_context * EDU44_PHASE_OWNER_BYTES;
    return edu43_phase_owner_active_valid(
               owners, before_contexts, lost_mailbox, lost_context,
               lost_slot, lost_entry_address, lost_generation,
               lost_request, lost_workload_generation, workload_checksum,
               lost_width, workload) &&
           !edu43_phase_owner_active_valid(
               owners, after_contexts, lost_mailbox, lost_context,
               lost_slot, lost_entry_address, lost_generation,
               lost_request, lost_workload_generation, workload_checksum,
               lost_width, workload) &&
           edu41_active_count(after_contexts) == 1ULL &&
           edu41_retirement_preserves_peer(
               before_contexts, after_contexts, lost_context) &&
           edu39_phase_owner_pair_valid(owners) &&
           edu39_phase_owner_matches(
               peer_owner, peer_context, peer_slot, peer_generation,
               peer_request, peer_workload_generation,
               EDU44_WORKLOAD_BYTES, workload_checksum,
               peer_width, workload) &&
           edu41_running_owner_valid(
               after_contexts, peer_slot, peer_entry_address,
               peer_generation, peer_request) &&
           edu41_mailbox_phase_owner(peer_mailbox) == peer_context;
}

int edu44_post_completion_retirement_blocks_redispatch(
    const edu44_u8 *before_contexts,
    const edu44_u8 *after_contexts,
    const edu44_u8 *completed_mailbox,
    const edu44_u8 *retired_mailbox,
    edu44_u32 retiring_context,
    edu44_u64 retiring_slot,
    edu44_u64 retiring_entry_address,
    edu44_u64 retiring_generation,
    edu44_u64 retiring_request,
    edu44_u64 peer_slot,
    edu44_u64 peer_entry_address,
    edu44_u64 peer_generation,
    edu44_u64 peer_request,
    edu44_u64 work_generation,
    edu44_u64 ap_error) {
    if (before_contexts == (const edu44_u8 *)0 ||
        after_contexts == (const edu44_u8 *)0 ||
        completed_mailbox == (const edu44_u8 *)0 ||
        retired_mailbox == (const edu44_u8 *)0 ||
        retiring_context >= EDU44_CONTEXT_COUNT) {
        return 0;
    }
    return edu40_mailbox_completion_valid(
               completed_mailbox, work_generation, EDU44_MODE_PHASE_BATCH,
               ap_error, retiring_context, retiring_slot,
               retiring_generation, retiring_request) &&
           edu40_mailbox_retired_valid(
               retired_mailbox, EDU44_MODE_PHASE_BATCH, ap_error,
               retiring_context, retiring_slot,
               retiring_generation, retiring_request) &&
           !edu40_mailbox_dispatch_valid(
               retired_mailbox, work_generation, EDU44_MODE_PHASE_BATCH,
               retiring_context, retiring_slot,
               retiring_generation, retiring_request) &&
           edu41_mailbox_phase_owner(retired_mailbox) ==
               EDU44_CONTEXT_NONE &&
           edu41_active_count(before_contexts) == EDU44_CONTEXT_COUNT &&
           edu41_active_count(after_contexts) == 1ULL &&
           !edu41_running_owner_valid(
               after_contexts, retiring_slot, retiring_entry_address,
               retiring_generation, retiring_request) &&
           edu41_running_owner_valid(
               after_contexts, peer_slot, peer_entry_address,
               peer_generation, peer_request) &&
           edu41_retirement_preserves_peer(
               before_contexts, after_contexts, retiring_context);
}
