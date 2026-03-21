#include <stdio.h>

typedef struct {
    int values[3];
    long long tail;
} Payload;

static Payload build(int seed) {
    Payload p = {{seed, seed + 2, seed + 4}, (long long)seed * 1000LL + 7LL};
    return p;
}

static Payload tweak(Payload in) {
    in.values[1] += in.values[0];
    in.values[2] += in.values[1];
    in.tail += (long long)in.values[2];
    return in;
}

int main(void) {
    Payload p = tweak(build(3));
    printf("%d %d %d %lld\n", p.values[0], p.values[1], p.values[2], p.tail);
    return 0;
}
