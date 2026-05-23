#include <stdio.h>

extern unsigned pointer_lifetime_epoch_bridge(unsigned seed);

int main(void) {
    printf("%u %u\n",
           pointer_lifetime_epoch_bridge(43u),
           pointer_lifetime_epoch_bridge(137u));
    return 0;
}
