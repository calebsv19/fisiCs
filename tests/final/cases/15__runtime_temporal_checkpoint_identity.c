#include "15__runtime_temporal_stable_fixture.h"

extern int edu44_unique_recovery_candidate_valid(
    const temporal_u8 *, const temporal_u8 *, const temporal_u8 *,
    const temporal_u8 *);
extern int printf(const char *, ...);

int main(void) {
    temporal_u8 before[320], restarted[320], old_lanes[1024], new_lanes[1024], entries[4096];
    temporal_fixture_checkpoint(before, restarted, old_lanes, new_lanes, entries);
    if (!edu44_unique_recovery_candidate_valid(old_lanes, new_lanes, entries, restarted) ||
        edu44_unique_recovery_candidate_valid(new_lanes, new_lanes, entries, restarted)) return 1;
    printf("OS-POST-EDU19 temporal-checkpoint-identity result=PASS\n");
    return 0;
}
