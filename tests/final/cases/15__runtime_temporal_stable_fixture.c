/* Shared byte-exact constructors from the established temporal probe. */
#define main temporal_fixture_reference_matrix_main
#include "../probes/runtime/15__probe_os_post_edu19_temporal_fault_sequence_matrix.c"
#undef main

void temporal_fixture_checkpoint(
    u8 *before, u8 *restarted, u8 *old_lanes, u8 *new_lanes, u8 *entries) {
    active_01(before);
    copy_bytes(restarted, before, 320U);
    activate(restarted, 0U, 2ULL, 0x1400ULL, 23ULL, 0x201ULL);
    build_lanes(old_lanes, 0U, 21U, 0x101ULL);
    build_lanes(new_lanes, 2U, 23U, 0x201ULL);
    zero_bytes(entries, 4096U);
}

void temporal_fixture_completion(
    u8 *before, u8 *after, u8 *completed_mailbox, u8 *retired_mailbox) {
    active_01(before);
    copy_bytes(after, before, 320U);
    retire_context(after, 0U);
    build_completed_mailbox(completed_mailbox, 0U, 0U, 21U, 0x101ULL, 0);
    build_completed_mailbox(retired_mailbox, 0U, 0U, 21U, 0x101ULL, 1);
}

void temporal_fixture_peer_loss(
    u8 *owners, u8 *before, u8 *after, u8 *corrupt_after,
    u8 *lost_mailbox, u8 *peer_mailbox, u8 *corrupt_peer_mailbox,
    u8 *workload, u32 *workload_checksum) {
    build_workload(workload);
    *workload_checksum = fnv(workload, 104U);
    active_01(before);
    copy_bytes(after, before, 320U);
    retire_context(after, 0U);
    copy_bytes(corrupt_after, after, 320U);
    corrupt_after[160U + 96U] ^= 1U;
    build_owners(owners, workload);
    build_dispatch_mailbox(lost_mailbox, 0U, 0U, 21U, 0x101ULL);
    build_dispatch_mailbox(peer_mailbox, 1U, 1U, 22U, 0x102ULL);
    copy_bytes(corrupt_peer_mailbox, peer_mailbox, 112U);
    put64(corrupt_peer_mailbox, 48U, 23ULL);
}

void temporal_fixture_cross_generation(
    u8 *before, u8 *restarted, u8 *old_lanes, u8 *new_lanes,
    u8 *entries, u8 *old_mailbox, u8 *new_mailbox) {
    temporal_fixture_checkpoint(before, restarted, old_lanes, new_lanes, entries);
    build_dispatch_mailbox(old_mailbox, 0U, 0U, 21U, 0x101ULL);
    build_dispatch_mailbox(new_mailbox, 0U, 2U, 23U, 0x201ULL);
}
