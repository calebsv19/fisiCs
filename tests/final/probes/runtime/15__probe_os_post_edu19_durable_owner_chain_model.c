/*
 * Hardware-blind compiler-side composition over immutable EDU-26, EDU-35,
 * EDU-37, EDU-39, EDU-40, and EDU-41 contracts.
 *
 * Each imported helper remains the frozen lesson-local owner. This unit
 * proves that generation, checkpoint, phase, mailbox, and runner identities
 * compose fail-closed. It is not OS source and does not model persistence,
 * hardware, or a general scheduler.
 */
typedef unsigned char edu43_u8;
typedef unsigned int edu43_u32;
typedef unsigned long long edu43_u64;

extern int edu26_queue_meta_valid(
    const edu43_u8 *metadata, edu43_u64 entry_lba,
    edu43_u32 entry_count);
extern edu43_u64 edu26_queue_entry_generation_action(
    const edu43_u8 *entry, const edu43_u8 *metadata, edu43_u64 slot);
extern int edu26_generation_reservable(edu43_u32 next_generation);
extern int edu26_ack_identity_valid(
    edu43_u32 slot, edu43_u32 state, edu43_u32 entry_generation,
    edu43_u64 entry_request, edu43_u32 acknowledged_generation,
    edu43_u64 acknowledged_request);
extern edu43_u64 edu35_checkpoint_snapshot_valid(const edu43_u8 *bytes);
extern int edu37_checkpoint_storage_valid(const edu43_u8 *lanes);
extern edu43_u64 edu37_checkpoint_lane_select(
    const edu43_u8 *lanes, const edu43_u8 *entries,
    edu43_u32 target_slot, edu43_u32 target_generation,
    edu43_u64 target_request);
extern int edu39_phase_owner_pair_valid(const edu43_u8 *owners);
extern int edu39_phase_owner_matches(
    const edu43_u8 *record, edu43_u32 context_id,
    edu43_u32 slot, edu43_u32 queue_generation,
    edu43_u64 request_id, edu43_u32 workload_generation,
    edu43_u32 workload_length, edu43_u32 workload_checksum,
    edu43_u32 width, const edu43_u8 *workload);
extern int edu40_mailbox_dispatch_valid(
    const edu43_u8 *mailbox, edu43_u64 work_generation,
    edu43_u32 mode, edu43_u64 selected_context,
    edu43_u64 selected_slot, edu43_u64 selected_queue_generation,
    edu43_u64 selected_request);
extern edu43_u64 edu41_active_count(const edu43_u8 *contexts);
extern int edu41_running_owner_valid(
    const edu43_u8 *contexts, edu43_u64 slot,
    edu43_u64 entry_address, edu43_u64 generation,
    edu43_u64 request);
extern edu43_u64 edu41_mailbox_phase_owner(const edu43_u8 *mailbox);
extern int edu41_retirement_preserves_peer(
    const edu43_u8 *before, const edu43_u8 *after,
    edu43_u32 retiring_context);

enum {
    EDU43_CONTEXT_COUNT = 2,
    EDU43_CHECKPOINT_BYTES = 512,
    EDU43_PHASE_OWNER_BYTES = 224,
    EDU43_WORKLOAD_BYTES = 104,
    EDU43_ENTRY_COUNT = 8,
    EDU43_MODE_PHASE_BATCH = 2
};

int edu43_generation_reuse_owner_valid(
    const edu43_u8 *before_contexts,
    const edu43_u8 *after_contexts,
    edu43_u32 retiring_context,
    edu43_u64 reclaimed_slot,
    edu43_u64 reclaimed_entry_address,
    edu43_u32 reclaimed_state,
    edu43_u32 reclaimed_generation,
    edu43_u64 reclaimed_request,
    edu43_u32 acknowledged_generation,
    edu43_u64 acknowledged_request,
    edu43_u32 next_generation,
    edu43_u64 new_request,
    edu43_u64 peer_slot,
    edu43_u64 peer_entry_address,
    edu43_u64 peer_generation,
    edu43_u64 peer_request) {
    if (retiring_context >= EDU43_CONTEXT_COUNT ||
        reclaimed_slot >= EDU43_ENTRY_COUNT ||
        (reclaimed_slot & 1ULL) != retiring_context ||
        new_request == 0 || new_request == reclaimed_request ||
        next_generation <= reclaimed_generation ||
        edu41_active_count(before_contexts) != EDU43_CONTEXT_COUNT ||
        edu41_active_count(after_contexts) != 1ULL ||
        !edu41_running_owner_valid(
            before_contexts, reclaimed_slot, reclaimed_entry_address,
            reclaimed_generation, reclaimed_request) ||
        !edu41_running_owner_valid(
            before_contexts, peer_slot, peer_entry_address,
            peer_generation, peer_request) ||
        edu41_running_owner_valid(
            after_contexts, reclaimed_slot, reclaimed_entry_address,
            reclaimed_generation, reclaimed_request) ||
        !edu41_running_owner_valid(
            after_contexts, peer_slot, peer_entry_address,
            peer_generation, peer_request) ||
        !edu41_retirement_preserves_peer(
            before_contexts, after_contexts, retiring_context) ||
        !edu26_ack_identity_valid(
            (edu43_u32)reclaimed_slot, reclaimed_state,
            reclaimed_generation, reclaimed_request,
            acknowledged_generation, acknowledged_request) ||
        !edu26_generation_reservable(next_generation)) {
        return 0;
    }
    return 1;
}

int edu43_checkpoint_owner_valid(
    const edu43_u8 *lanes,
    const edu43_u8 *entries,
    const edu43_u8 *contexts,
    edu43_u32 context,
    edu43_u32 slot,
    edu43_u64 entry_address,
    edu43_u32 generation,
    edu43_u64 request,
    edu43_u32 expected_lane) {
    const edu43_u8 *snapshot;
    if (lanes == (const edu43_u8 *)0 ||
        entries == (const edu43_u8 *)0 ||
        contexts == (const edu43_u8 *)0 ||
        context >= EDU43_CONTEXT_COUNT ||
        slot >= EDU43_ENTRY_COUNT ||
        expected_lane >= 2 ||
        (slot & 1U) != context) return 0;
    snapshot = lanes + expected_lane * EDU43_CHECKPOINT_BYTES;
    return edu37_checkpoint_storage_valid(lanes) &&
           edu35_checkpoint_snapshot_valid(snapshot) != 0 &&
           edu37_checkpoint_lane_select(
               lanes, entries, slot, generation, request) == expected_lane &&
           edu41_running_owner_valid(
               contexts, slot, entry_address, generation, request);
}

int edu43_phase_owner_active_valid(
    const edu43_u8 *owners,
    const edu43_u8 *contexts,
    const edu43_u8 *mailbox,
    edu43_u32 context,
    edu43_u32 slot,
    edu43_u64 entry_address,
    edu43_u32 queue_generation,
    edu43_u64 request,
    edu43_u32 workload_generation,
    edu43_u32 workload_checksum,
    edu43_u32 width,
    const edu43_u8 *workload) {
    const edu43_u8 *record;
    if (owners == (const edu43_u8 *)0 ||
        contexts == (const edu43_u8 *)0 ||
        mailbox == (const edu43_u8 *)0 ||
        workload == (const edu43_u8 *)0 ||
        context >= EDU43_CONTEXT_COUNT ||
        slot >= EDU43_ENTRY_COUNT ||
        (slot & 1U) != context) return 0;
    record = owners + context * EDU43_PHASE_OWNER_BYTES;
    return edu39_phase_owner_pair_valid(owners) &&
           edu39_phase_owner_matches(
               record, context, slot, queue_generation, request,
               workload_generation, EDU43_WORKLOAD_BYTES,
               workload_checksum, width, workload) &&
           edu41_active_count(contexts) == EDU43_CONTEXT_COUNT &&
           edu41_running_owner_valid(
               contexts, slot, entry_address,
               queue_generation, request) &&
           edu41_mailbox_phase_owner(mailbox) == context;
}

int edu43_durable_owner_chain_valid(
    const edu43_u8 *metadata,
    edu43_u64 entry_lba,
    const edu43_u8 *entry,
    const edu43_u8 *lanes,
    const edu43_u8 *entries,
    const edu43_u8 *owners,
    const edu43_u8 *contexts,
    const edu43_u8 *mailbox,
    edu43_u32 context,
    edu43_u32 slot,
    edu43_u64 entry_address,
    edu43_u32 queue_generation,
    edu43_u64 request,
    edu43_u32 workload_generation,
    edu43_u32 workload_checksum,
    edu43_u32 width,
    const edu43_u8 *workload,
    edu43_u32 expected_lane,
    edu43_u64 work_generation) {
    if (!edu26_queue_meta_valid(metadata, entry_lba, EDU43_ENTRY_COUNT) ||
        edu26_queue_entry_generation_action(
            entry, metadata, slot) != 0 ||
        !edu43_checkpoint_owner_valid(
            lanes, entries, contexts, context, slot, entry_address,
            queue_generation, request, expected_lane) ||
        !edu43_phase_owner_active_valid(
            owners, contexts, mailbox, context, slot, entry_address,
            queue_generation, request, workload_generation,
            workload_checksum, width, workload)) return 0;
    return edu40_mailbox_dispatch_valid(
        mailbox, work_generation, EDU43_MODE_PHASE_BATCH,
        context, slot, queue_generation, request);
}
