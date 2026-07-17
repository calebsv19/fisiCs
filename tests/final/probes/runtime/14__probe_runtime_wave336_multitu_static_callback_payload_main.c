#include <stdio.h>

typedef struct BridgeReport {
    unsigned short guard;
    unsigned char lane0;
    unsigned char lane3;
    unsigned calls;
    unsigned fold;
} BridgeReport;

void wave336_bridge_seed(unsigned seed);
BridgeReport wave336_bridge_apply(unsigned salt);

int main(void) {
    BridgeReport first;
    BridgeReport second;

    wave336_bridge_seed(7u);
    first = wave336_bridge_apply(11u);
    second = wave336_bridge_apply(29u);
    printf("multitu-static %u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)first.guard, (unsigned)first.lane0, (unsigned)first.lane3,
           first.calls, first.fold, (unsigned)second.guard,
           (unsigned)second.lane0, (unsigned)second.lane3,
           second.calls, second.fold);
    return 0;
}
