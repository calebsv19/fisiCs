#include "15__runtime_temporal_stable_fixture.h"

extern int edu44_stale_evidence_cross_generation_rejected(
    const temporal_u8 *, const temporal_u8 *, const temporal_u8 *,
    const temporal_u8 *, const temporal_u8 *, const temporal_u8 *,
    const temporal_u8 *, temporal_u32, temporal_u64);
extern int printf(const char *, ...);

int main(void) {
    temporal_u8 before[320], restarted[320], old_lanes[1024], new_lanes[1024], entries[4096];
    temporal_u8 old_mailbox[112], new_mailbox[112];
    temporal_fixture_cross_generation(before, restarted, old_lanes, new_lanes, entries, old_mailbox, new_mailbox);
    if (!edu44_stale_evidence_cross_generation_rejected(
            old_lanes, new_lanes, entries, before, restarted, old_mailbox, new_mailbox, 31U, 0x201ULL) ||
        edu44_stale_evidence_cross_generation_rejected(
            old_lanes, new_lanes, entries, before, restarted, old_mailbox, new_mailbox, 32U, 0x301ULL)) return 1;
    printf("OS-POST-EDU19 temporal-stale-ack result=PASS\n");
    return 0;
}
