#include <stdio.h>

extern unsigned owner_window_stale_handoff_reclaim(unsigned seed);

int main(void) {
    printf("%u %u\n",
           owner_window_stale_handoff_reclaim(59u),
           owner_window_stale_handoff_reclaim(127u));
    return 0;
}
