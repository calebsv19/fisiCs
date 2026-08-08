#include "15__runtime_temporal_stable_fixture.h"

extern int edu44_pair_owner_loss_peer_corruption_rejected(
    const temporal_u8 *, const temporal_u8 *, const temporal_u8 *, const temporal_u8 *,
    const temporal_u8 *, const temporal_u8 *, const temporal_u8 *, temporal_u32,
    const temporal_u8 *);
extern int printf(const char *, ...);

int main(void) {
    temporal_u8 owners[448], before[320], after[320], corrupt_after[320], workload[104];
    temporal_u8 lost_mailbox[112], peer_mailbox[112], corrupt_peer_mailbox[112];
    temporal_u32 checksum;
    temporal_fixture_peer_loss(owners, before, after, corrupt_after, lost_mailbox,
                               peer_mailbox, corrupt_peer_mailbox, workload, &checksum);
    if (!edu44_pair_owner_loss_peer_corruption_rejected(
            owners, before, after, corrupt_after, lost_mailbox, peer_mailbox,
            corrupt_peer_mailbox, checksum, workload) ||
        edu44_pair_owner_loss_peer_corruption_rejected(
            owners, before, after, after, lost_mailbox, peer_mailbox,
            peer_mailbox, checksum, workload)) return 1;
    printf("OS-POST-EDU19 temporal-peer-preservation result=PASS\n");
    return 0;
}
