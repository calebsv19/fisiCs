#include <stdio.h>

typedef struct {
    double x;
    double y;
    int z;
} MixRet;

static MixRet make_value(int seed) {
    MixRet value;
    value.x = (double)seed + 0.5;
    value.y = (double)seed * 2.25;
    value.z = seed + 7;
    return value;
}

int main(void) {
    MixRet value = make_value(4);
    long long score = (long long)(value.x * 4.0) +
                      (long long)(value.y * 4.0) +
                      (long long)value.z;
    printf("%lld\n", score);
    return 0;
}
