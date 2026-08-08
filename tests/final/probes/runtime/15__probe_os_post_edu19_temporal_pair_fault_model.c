/*
 * Hardware-blind compiler probe that composes the first EDU-44 temporal
 * model into paired-fault and cross-generation rejection checks.
 *
 * This is not immutable OS source and not an EDU-45 operating-system claim.
 * It checks that individually valid lifecycle observations cannot be spliced
 * into a valid sequence when their owner generations, ordering, or peer
 * evidence disagree.
 */
typedef unsigned char pair_u8;
typedef unsigned int pair_u32;
typedef unsigned long long pair_u64;

extern int edu44_pre_ack_interruption_blocks_reuse(
    const pair_u8 *before_contexts, const pair_u8 *after_contexts,
    pair_u32 retiring_context, pair_u64 reclaimed_slot,
    pair_u64 reclaimed_entry_address, pair_u32 reclaimed_state,
    pair_u32 reclaimed_generation, pair_u64 reclaimed_request,
    pair_u32 observed_ack_generation, pair_u64 observed_ack_request,
    pair_u32 next_generation, pair_u64 new_request,
    pair_u64 peer_slot, pair_u64 peer_entry_address,
    pair_u64 peer_generation, pair_u64 peer_request);
extern int edu44_post_checkpoint_restart_rejects_stale(
    const pair_u8 *old_lanes, const pair_u8 *new_lanes,
    const pair_u8 *entries, const pair_u8 *before_contexts,
    const pair_u8 *restarted_contexts, pair_u32 context,
    pair_u32 old_slot, pair_u64 old_entry_address,
    pair_u32 old_generation, pair_u64 old_request, pair_u32 old_lane,
    pair_u32 new_slot, pair_u64 new_entry_address,
    pair_u32 new_generation, pair_u64 new_request, pair_u32 new_lane,
    pair_u64 peer_slot, pair_u64 peer_entry_address,
    pair_u64 peer_generation, pair_u64 peer_request);
extern int edu44_mid_phase_owner_loss_preserves_peer(
    const pair_u8 *owners, const pair_u8 *before_contexts,
    const pair_u8 *after_contexts, const pair_u8 *lost_mailbox,
    const pair_u8 *peer_mailbox, pair_u32 lost_context,
    pair_u32 lost_slot, pair_u64 lost_entry_address,
    pair_u32 lost_generation, pair_u64 lost_request,
    pair_u32 lost_workload_generation, pair_u32 lost_width,
    pair_u32 peer_slot, pair_u64 peer_entry_address,
    pair_u32 peer_generation, pair_u64 peer_request,
    pair_u32 peer_workload_generation, pair_u32 peer_width,
    pair_u32 workload_checksum, const pair_u8 *workload);
extern int edu44_post_completion_retirement_blocks_redispatch(
    const pair_u8 *before_contexts, const pair_u8 *after_contexts,
    const pair_u8 *completed_mailbox, const pair_u8 *retired_mailbox,
    pair_u32 retiring_context, pair_u64 retiring_slot,
    pair_u64 retiring_entry_address, pair_u64 retiring_generation,
    pair_u64 retiring_request, pair_u64 peer_slot,
    pair_u64 peer_entry_address, pair_u64 peer_generation,
    pair_u64 peer_request, pair_u64 work_generation, pair_u64 ap_error);
extern int edu26_ack_identity_valid(
    pair_u32 slot, pair_u32 state, pair_u32 entry_generation,
    pair_u64 entry_request, pair_u32 acknowledged_generation,
    pair_u64 acknowledged_request);
extern int edu40_mailbox_dispatch_valid(
    const pair_u8 *mailbox, pair_u64 work_generation,
    pair_u32 mode, pair_u64 selected_context,
    pair_u64 selected_slot, pair_u64 selected_queue_generation,
    pair_u64 selected_request);
extern int edu43_checkpoint_owner_valid(
    const pair_u8 *lanes, const pair_u8 *entries,
    const pair_u8 *contexts, pair_u32 context, pair_u32 slot,
    pair_u64 entry_address, pair_u32 generation,
    pair_u64 request, pair_u32 expected_lane);

enum {
    PAIR_ENTRY_COMPLETE = 3,
    PAIR_MODE_PHASE_BATCH = 2
};

static pair_u32 pair_read32(const pair_u8 *bytes) {
    return (pair_u32)bytes[0] |
           ((pair_u32)bytes[1] << 8U) |
           ((pair_u32)bytes[2] << 16U) |
           ((pair_u32)bytes[3] << 24U);
}

static pair_u64 pair_read64(const pair_u8 *bytes) {
    return (pair_u64)pair_read32(bytes) |
           ((pair_u64)pair_read32(bytes + 4) << 32U);
}

static int pair_checkpoint_record_matches(
    const pair_u8 *lanes, pair_u32 lane, pair_u32 slot,
    pair_u32 generation, pair_u64 request) {
    const pair_u8 *record;
    if (lanes == (const pair_u8 *)0 || lane >= 2U) return 0;
    record = lanes + lane * 512U;
    return pair_read32(record + 16U) == slot &&
           pair_read32(record + 20U) == generation &&
           pair_read64(record + 24U) == request;
}

int edu44_pair_checkpoint_interruption_blocks_reuse(
    const pair_u8 *old_lanes, const pair_u8 *new_lanes,
    const pair_u8 *entries, const pair_u8 *checkpoint_before,
    const pair_u8 *restarted_contexts,
    const pair_u8 *reuse_before, const pair_u8 *reuse_after,
    pair_u32 observed_ack_generation, pair_u64 observed_ack_request) {
    return edu44_post_checkpoint_restart_rejects_stale(
               old_lanes, new_lanes, entries,
               checkpoint_before, restarted_contexts,
               0U, 0U, 0x1000ULL, 21U, 0x101ULL, 0U,
               2U, 0x1400ULL, 23U, 0x201ULL, 0U,
               1ULL, 0x1200ULL, 22ULL, 0x102ULL) &&
           edu44_pre_ack_interruption_blocks_reuse(
               reuse_before, reuse_after, 0U,
               4ULL, 0x1800ULL, PAIR_ENTRY_COMPLETE,
               31U, 0x201ULL,
               observed_ack_generation, observed_ack_request,
               32U, 0x301ULL,
               5ULL, 0x1A00ULL, 32ULL, 0x202ULL);
}

int edu44_pair_completion_retirement_order_valid(
    const pair_u8 *before_contexts, const pair_u8 *after_contexts,
    const pair_u8 *completed_mailbox, const pair_u8 *retired_mailbox) {
    return edu44_post_completion_retirement_blocks_redispatch(
               before_contexts, after_contexts,
               completed_mailbox, retired_mailbox,
               0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL,
               1ULL, 0x1200ULL, 22ULL, 0x102ULL, 7ULL, 0ULL) &&
           !edu44_post_completion_retirement_blocks_redispatch(
               before_contexts, after_contexts,
               retired_mailbox, completed_mailbox,
               0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL,
               1ULL, 0x1200ULL, 22ULL, 0x102ULL, 7ULL, 0ULL);
}

int edu44_pair_owner_loss_peer_corruption_rejected(
    const pair_u8 *owners, const pair_u8 *before_contexts,
    const pair_u8 *clean_after_contexts,
    const pair_u8 *corrupt_after_contexts,
    const pair_u8 *lost_mailbox, const pair_u8 *clean_peer_mailbox,
    const pair_u8 *corrupt_peer_mailbox,
    pair_u32 workload_checksum, const pair_u8 *workload) {
    int clean;
    int corrupt;
    clean = edu44_mid_phase_owner_loss_preserves_peer(
        owners, before_contexts, clean_after_contexts,
        lost_mailbox, clean_peer_mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U, 1U,
        1U, 0x1200ULL, 22U, 0x102ULL, 32U, 2U,
        workload_checksum, workload);
    corrupt = edu44_mid_phase_owner_loss_preserves_peer(
        owners, before_contexts, corrupt_after_contexts,
        lost_mailbox, corrupt_peer_mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U, 1U,
        1U, 0x1200ULL, 22U, 0x102ULL, 32U, 2U,
        workload_checksum, workload);
    return clean && !corrupt;
}

int edu44_stale_evidence_cross_generation_rejected(
    const pair_u8 *old_lanes, const pair_u8 *new_lanes,
    const pair_u8 *entries, const pair_u8 *before_contexts,
    const pair_u8 *restarted_contexts,
    const pair_u8 *old_mailbox, const pair_u8 *new_mailbox,
    pair_u32 old_ack_generation, pair_u64 old_ack_request) {
    return edu44_post_checkpoint_restart_rejects_stale(
               old_lanes, new_lanes, entries,
               before_contexts, restarted_contexts,
               0U, 0U, 0x1000ULL, 21U, 0x101ULL, 0U,
               2U, 0x1400ULL, 23U, 0x201ULL, 0U,
               1ULL, 0x1200ULL, 22ULL, 0x102ULL) &&
           edu26_ack_identity_valid(
               4U, PAIR_ENTRY_COMPLETE, 31U, 0x201ULL,
               old_ack_generation, old_ack_request) &&
           !edu26_ack_identity_valid(
               4U, PAIR_ENTRY_COMPLETE, 32U, 0x301ULL,
               old_ack_generation, old_ack_request) &&
           edu40_mailbox_dispatch_valid(
               old_mailbox, 7ULL, PAIR_MODE_PHASE_BATCH,
               0ULL, 0ULL, 21ULL, 0x101ULL) &&
           !edu40_mailbox_dispatch_valid(
               old_mailbox, 7ULL, PAIR_MODE_PHASE_BATCH,
               0ULL, 2ULL, 23ULL, 0x201ULL) &&
           edu40_mailbox_dispatch_valid(
               new_mailbox, 7ULL, PAIR_MODE_PHASE_BATCH,
               0ULL, 2ULL, 23ULL, 0x201ULL);
}

int edu44_unique_recovery_candidate_valid(
    const pair_u8 *stale_lanes, const pair_u8 *exact_lanes,
    const pair_u8 *entries, const pair_u8 *restarted_contexts) {
    int stale_valid;
    int exact_valid;
    stale_valid = edu43_checkpoint_owner_valid(
        stale_lanes, entries, restarted_contexts,
        0U, 2U, 0x1400ULL, 23U, 0x201ULL, 0U) &&
        pair_checkpoint_record_matches(
            stale_lanes, 0U, 2U, 23U, 0x201ULL);
    exact_valid = edu43_checkpoint_owner_valid(
        exact_lanes, entries, restarted_contexts,
        0U, 2U, 0x1400ULL, 23U, 0x201ULL, 0U) &&
        pair_checkpoint_record_matches(
            exact_lanes, 0U, 2U, 23U, 0x201ULL);
    return !stale_valid && exact_valid;
}
