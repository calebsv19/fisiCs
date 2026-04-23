#include <stdio.h>

extern int axis1_w2_extent_table[];
extern int axis1_w2_extent_count(void);
extern int axis1_w2_extent_lane_mix(int idx, int salt);
extern int axis1_w2_extent_bridge_step(int idx, int salt);

int main(void) {
    unsigned digest = 5381u;
    int count = axis1_w2_extent_count();
    int i;

    for (i = 0; i < 15; ++i) {
        int idx = (i * 3 + 1) % count;
        int lane = axis1_w2_extent_lane_mix(idx, 19 + i * 2);
        int bridge = axis1_w2_extent_bridge_step((idx + 2) % count, 7 + i * 5);
        digest = ((digest << 5) + digest) ^ (unsigned)(
            lane + bridge + axis1_w2_extent_table[(idx + 1) % count]);
    }

    printf("%d %d %u\n", count, axis1_w2_extent_table[3], digest);
    return 0;
}
