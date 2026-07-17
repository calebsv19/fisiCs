#include <stdio.h>

struct NextAbiLarge {
    int lane[10];
    int tag;
    int salt;
};

typedef struct NextAbiLarge (*NextAbiLargeFn)(struct NextAbiLarge, int);

struct NextAbiLarge nextbatch_large_seed(int base);
struct NextAbiLarge nextbatch_large_bridge(struct NextAbiLarge in, NextAbiLargeFn cb, int rounds);
unsigned nextbatch_large_fold(struct NextAbiLarge value);

static struct NextAbiLarge local_callback(struct NextAbiLarge in, int step) {
    struct NextAbiLarge out = in;
    int i;

    for (i = 0; i < 10; ++i) {
        out.lane[i] = in.lane[(i + step) % 10] + step * (i + 1);
    }
    out.tag = in.tag ^ (step * 41 + out.lane[step % 10]);
    out.salt = in.salt + step * 97 - out.lane[(step + 3) % 10];
    return out;
}

int main(void) {
    struct NextAbiLarge seed = nextbatch_large_seed(23);
    struct NextAbiLarge out = nextbatch_large_bridge(seed, local_callback, 7);
    unsigned folded = nextbatch_large_fold(out);

    printf("%u %d %d %d\n", folded, out.lane[2], out.tag, out.salt);
    return 0;
}
