#include <stdio.h>

extern unsigned axis1_wave17_ptrtable_tu_order_checkpoint_alias_bridge(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave17_ptrtable_tu_order_checkpoint_alias_bridge(73u),
           axis1_wave17_ptrtable_tu_order_checkpoint_alias_bridge(149u));
    return 0;
}
