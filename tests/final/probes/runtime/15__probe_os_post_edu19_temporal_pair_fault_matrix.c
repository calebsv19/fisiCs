/* Reuse the established byte-exact EDU-44 fixture constructors. */
#define main edu44_first_temporal_wave_fixture_main
#include "15__probe_os_post_edu19_temporal_fault_sequence_matrix.c"
#undef main

extern int edu44_pair_checkpoint_interruption_blocks_reuse(
    const u8 *, const u8 *, const u8 *, const u8 *, const u8 *,
    const u8 *, const u8 *, u32, u64);
extern int edu44_pair_completion_retirement_order_valid(
    const u8 *, const u8 *, const u8 *, const u8 *);
extern int edu44_pair_owner_loss_peer_corruption_rejected(
    const u8 *, const u8 *, const u8 *, const u8 *,
    const u8 *, const u8 *, const u8 *, u32, const u8 *);
extern int edu44_stale_evidence_cross_generation_rejected(
    const u8 *, const u8 *, const u8 *, const u8 *, const u8 *,
    const u8 *, const u8 *, u32, u64);
extern int edu44_unique_recovery_candidate_valid(
    const u8 *, const u8 *, const u8 *, const u8 *);

static int pair_checks;
static u32 pair_digest = 2166136261U;

static int pair_expect(u32 id, u64 actual, u64 expected) {
    pair_checks = pair_checks + 1;
    pair_digest = (pair_digest ^ id) * 16777619U;
    pair_digest = (pair_digest ^ (u32)actual) * 16777619U;
    pair_digest = (pair_digest ^ (u32)(actual >> 32U)) * 16777619U;
    return actual == expected ? 0 : (int)id;
}

int main(void) {
    u8 checkpoint_before[320], restarted[320], candidate_contexts[320];
    u8 reuse_before[320], reuse_after[320];
    u8 old_lanes[1024], new_lanes[1024], candidate_lanes[1024];
    u8 entries[4096], owners[448], workload[104];
    u8 lost_mailbox[112], peer_mailbox[112], corrupt_peer_mailbox[112];
    u8 completed_mailbox[112], retired_mailbox[112], candidate_mailbox[112];
    u8 old_mailbox[112], new_mailbox[112];
    u32 workload_checksum;
    int failure;

#define PAIR_RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = pair_expect((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    active_01(checkpoint_before);
    copy_bytes(restarted, checkpoint_before, 320U);
    activate(restarted, 0U, 2ULL, 0x1400ULL, 23ULL, 0x201ULL);
    build_lanes(old_lanes, 0U, 21U, 0x101ULL);
    build_lanes(new_lanes, 2U, 23U, 0x201ULL);
    zero_bytes(entries, 4096U);
    active_45(reuse_before);
    copy_bytes(reuse_after, reuse_before, 320U);
    retire_context(reuse_after, 0U);
    PAIR_RUN(1U, edu44_pair_checkpoint_interruption_blocks_reuse(
        old_lanes, new_lanes, entries, checkpoint_before, restarted,
        reuse_before, reuse_after, 30U, 0x201ULL), 1U);
    PAIR_RUN(2U, edu44_pair_checkpoint_interruption_blocks_reuse(
        old_lanes, new_lanes, entries, checkpoint_before, restarted,
        reuse_before, reuse_after, 31U, 0x201ULL), 0U);
    copy_bytes(candidate_lanes, new_lanes, 1024U);
    candidate_lanes[508U] ^= 1U;
    PAIR_RUN(3U, edu44_pair_checkpoint_interruption_blocks_reuse(
        old_lanes, candidate_lanes, entries, checkpoint_before, restarted,
        reuse_before, reuse_after, 30U, 0x201ULL), 0U);
    copy_bytes(candidate_contexts, reuse_after, 320U);
    candidate_contexts[160U + 80U] ^= 1U;
    PAIR_RUN(4U, edu44_pair_checkpoint_interruption_blocks_reuse(
        old_lanes, new_lanes, entries, checkpoint_before, restarted,
        reuse_before, candidate_contexts, 30U, 0x201ULL), 0U);
    copy_bytes(candidate_contexts, restarted, 320U);
    put64(candidate_contexts + 160U, 32U, 23ULL);
    PAIR_RUN(5U, edu44_pair_checkpoint_interruption_blocks_reuse(
        old_lanes, new_lanes, entries, checkpoint_before, candidate_contexts,
        reuse_before, reuse_after, 30U, 0x201ULL), 0U);

    active_01(checkpoint_before);
    copy_bytes(restarted, checkpoint_before, 320U);
    retire_context(restarted, 0U);
    build_completed_mailbox(completed_mailbox, 0U, 0U, 21U, 0x101ULL, 0);
    build_completed_mailbox(retired_mailbox, 0U, 0U, 21U, 0x101ULL, 1);
    PAIR_RUN(6U, edu44_pair_completion_retirement_order_valid(
        checkpoint_before, restarted, completed_mailbox, retired_mailbox), 1U);
    PAIR_RUN(7U, edu44_pair_completion_retirement_order_valid(
        checkpoint_before, restarted, retired_mailbox, completed_mailbox), 0U);
    PAIR_RUN(8U, edu44_pair_completion_retirement_order_valid(
        checkpoint_before, checkpoint_before,
        completed_mailbox, retired_mailbox), 0U);
    copy_bytes(candidate_mailbox, retired_mailbox, 112U);
    put64(candidate_mailbox, 0U, 1ULL);
    PAIR_RUN(9U, edu44_pair_completion_retirement_order_valid(
        checkpoint_before, restarted, completed_mailbox, candidate_mailbox), 0U);
    copy_bytes(candidate_contexts, restarted, 320U);
    candidate_contexts[160U + 104U] ^= 1U;
    PAIR_RUN(10U, edu44_pair_completion_retirement_order_valid(
        checkpoint_before, candidate_contexts,
        completed_mailbox, retired_mailbox), 0U);

    build_workload(workload);
    workload_checksum = fnv(workload, 104U);
    active_01(checkpoint_before);
    copy_bytes(restarted, checkpoint_before, 320U);
    retire_context(restarted, 0U);
    copy_bytes(candidate_contexts, restarted, 320U);
    candidate_contexts[160U + 96U] ^= 1U;
    build_owners(owners, workload);
    build_dispatch_mailbox(lost_mailbox, 0U, 0U, 21U, 0x101ULL);
    build_dispatch_mailbox(peer_mailbox, 1U, 1U, 22U, 0x102ULL);
    copy_bytes(corrupt_peer_mailbox, peer_mailbox, 112U);
    put64(corrupt_peer_mailbox, 48U, 23ULL);
    PAIR_RUN(11U, edu44_pair_owner_loss_peer_corruption_rejected(
        owners, checkpoint_before, restarted, candidate_contexts,
        lost_mailbox, peer_mailbox, corrupt_peer_mailbox,
        workload_checksum, workload), 1U);
    PAIR_RUN(12U, edu44_pair_owner_loss_peer_corruption_rejected(
        owners, checkpoint_before, restarted, restarted,
        lost_mailbox, peer_mailbox, peer_mailbox,
        workload_checksum, workload), 0U);
    PAIR_RUN(13U, edu44_pair_owner_loss_peer_corruption_rejected(
        owners, checkpoint_before, checkpoint_before, candidate_contexts,
        lost_mailbox, peer_mailbox, corrupt_peer_mailbox,
        workload_checksum, workload), 0U);
    PAIR_RUN(14U, edu44_pair_owner_loss_peer_corruption_rejected(
        owners, checkpoint_before, restarted, candidate_contexts,
        peer_mailbox, peer_mailbox, corrupt_peer_mailbox,
        workload_checksum, workload), 0U);
    copy_bytes(candidate_lanes, owners, 448U);
    candidate_lanes[224U + 28U] ^= 1U;
    PAIR_RUN(15U, edu44_pair_owner_loss_peer_corruption_rejected(
        candidate_lanes, checkpoint_before, restarted, candidate_contexts,
        lost_mailbox, peer_mailbox, corrupt_peer_mailbox,
        workload_checksum, workload), 0U);

    active_01(checkpoint_before);
    copy_bytes(restarted, checkpoint_before, 320U);
    activate(restarted, 0U, 2ULL, 0x1400ULL, 23ULL, 0x201ULL);
    build_lanes(old_lanes, 0U, 21U, 0x101ULL);
    build_lanes(new_lanes, 2U, 23U, 0x201ULL);
    build_dispatch_mailbox(old_mailbox, 0U, 0U, 21U, 0x101ULL);
    build_dispatch_mailbox(new_mailbox, 0U, 2U, 23U, 0x201ULL);
    PAIR_RUN(16U, edu44_stale_evidence_cross_generation_rejected(
        old_lanes, new_lanes, entries, checkpoint_before, restarted,
        old_mailbox, new_mailbox, 31U, 0x201ULL), 1U);
    PAIR_RUN(17U, edu44_stale_evidence_cross_generation_rejected(
        old_lanes, new_lanes, entries, checkpoint_before, restarted,
        old_mailbox, new_mailbox, 32U, 0x301ULL), 0U);
    PAIR_RUN(18U, edu44_stale_evidence_cross_generation_rejected(
        old_lanes, new_lanes, entries, checkpoint_before, restarted,
        new_mailbox, new_mailbox, 31U, 0x201ULL), 0U);
    PAIR_RUN(19U, edu44_stale_evidence_cross_generation_rejected(
        old_lanes, new_lanes, entries, checkpoint_before, restarted,
        old_mailbox, old_mailbox, 31U, 0x201ULL), 0U);
    PAIR_RUN(20U, edu44_stale_evidence_cross_generation_rejected(
        old_lanes, old_lanes, entries, checkpoint_before, restarted,
        old_mailbox, new_mailbox, 31U, 0x201ULL), 0U);

    PAIR_RUN(21U, edu44_unique_recovery_candidate_valid(
        old_lanes, new_lanes, entries, restarted), 1U);
    PAIR_RUN(22U, edu44_unique_recovery_candidate_valid(
        new_lanes, new_lanes, entries, restarted), 0U);
    PAIR_RUN(23U, edu44_unique_recovery_candidate_valid(
        old_lanes, old_lanes, entries, restarted), 0U);
    copy_bytes(candidate_lanes, new_lanes, 1024U);
    candidate_lanes[508U] ^= 1U;
    PAIR_RUN(24U, edu44_unique_recovery_candidate_valid(
        old_lanes, candidate_lanes, entries, restarted), 0U);
    copy_bytes(candidate_contexts, restarted, 320U);
    put64(candidate_contexts, 16U, 3ULL);
    PAIR_RUN(25U, edu44_unique_recovery_candidate_valid(
        old_lanes, new_lanes, entries, candidate_contexts), 0U);

    printf(
        "OS-POST-EDU19 temporal-pair-fault "
        "basis=temporal-fault-sequence-v1 vectors=%d digest=%u result=PASS\n",
        pair_checks, pair_digest);
    return 0;
#undef PAIR_RUN
}
