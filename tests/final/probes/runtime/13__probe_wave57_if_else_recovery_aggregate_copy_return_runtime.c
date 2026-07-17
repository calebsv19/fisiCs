#include <stdio.h>

typedef struct Wave57Packet {
    int lane[3];
} Wave57Packet;

static Wave57Packet wave57_build(int seed) {
    Wave57Packet packet = {{seed, seed + 4, seed * 3}};
    return packet;
}

static Wave57Packet wave57_recover_then_copy(int seed) {
#if WAVE57_MALFORMED
    if () {
        seed += 100;
    } else {
        seed += 0;
    }
#else
    if (0) {
        seed += 100;
    } else {
        seed += 0;
    }
#endif

    Wave57Packet built = wave57_build(seed);
    Wave57Packet copied = built;
    return copied;
}

int main(void) {
    Wave57Packet returned = wave57_recover_then_copy(7);
    Wave57Packet final_copy = returned;

    printf("%d %d %d %d\n",
           final_copy.lane[0],
           final_copy.lane[1],
           final_copy.lane[2],
           final_copy.lane[0] + final_copy.lane[1] + final_copy.lane[2]);
    return 0;
}
