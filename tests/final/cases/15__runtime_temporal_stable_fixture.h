#ifndef FISICS_TEMPORAL_STABLE_FIXTURE_H
#define FISICS_TEMPORAL_STABLE_FIXTURE_H

typedef unsigned char temporal_u8;
typedef unsigned int temporal_u32;
typedef unsigned long long temporal_u64;

void temporal_fixture_checkpoint(
    temporal_u8 *before, temporal_u8 *restarted,
    temporal_u8 *old_lanes, temporal_u8 *new_lanes,
    temporal_u8 *entries);
void temporal_fixture_completion(
    temporal_u8 *before, temporal_u8 *after,
    temporal_u8 *completed_mailbox, temporal_u8 *retired_mailbox);
void temporal_fixture_peer_loss(
    temporal_u8 *owners, temporal_u8 *before, temporal_u8 *after,
    temporal_u8 *corrupt_after, temporal_u8 *lost_mailbox,
    temporal_u8 *peer_mailbox, temporal_u8 *corrupt_peer_mailbox,
    temporal_u8 *workload, temporal_u32 *workload_checksum);
void temporal_fixture_cross_generation(
    temporal_u8 *before, temporal_u8 *restarted,
    temporal_u8 *old_lanes, temporal_u8 *new_lanes,
    temporal_u8 *entries, temporal_u8 *old_mailbox,
    temporal_u8 *new_mailbox);

#endif
