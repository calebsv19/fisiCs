#include "15__runtime_temporal_stable_fixture.h"

extern int edu44_pair_completion_retirement_order_valid(
    const temporal_u8 *, const temporal_u8 *, const temporal_u8 *, const temporal_u8 *);
extern int printf(const char *, ...);

int main(void) {
    temporal_u8 before[320], after[320], completed_mailbox[112], retired_mailbox[112];
    temporal_fixture_completion(before, after, completed_mailbox, retired_mailbox);
    if (!edu44_pair_completion_retirement_order_valid(before, after, completed_mailbox, retired_mailbox) ||
        edu44_pair_completion_retirement_order_valid(before, after, retired_mailbox, completed_mailbox)) return 1;
    printf("OS-POST-EDU19 temporal-retirement-order result=PASS\n");
    return 0;
}
