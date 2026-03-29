#include <stdio.h>

unsigned multitu_lane_hash(unsigned lane, unsigned seed);
unsigned multitu_lane_mix(unsigned acc, unsigned lane_hash, unsigned step);

int main(void) {
    unsigned pass1 = 2166136261u;
    unsigned pass2 = 2166136261u;
    unsigned lane;
    unsigned step = 0u;

    for (lane = 0u; lane < 8u; ++lane) {
        unsigned h1 = multitu_lane_hash(lane, lane * 17u + 3u);
        unsigned h2 = multitu_lane_hash(lane, lane * 17u + 3u);
        if (h1 != h2) {
            printf("multitu-repeatability-fail %u %u %u\n", lane, h1, h2);
            return 23;
        }
        pass1 = multitu_lane_mix(pass1, h1, step++);
        pass2 = multitu_lane_mix(pass2, multitu_lane_hash(lane, lane + 101u), step++);
    }

    printf("%u %u\n", pass1, pass2);
    return 0;
}
