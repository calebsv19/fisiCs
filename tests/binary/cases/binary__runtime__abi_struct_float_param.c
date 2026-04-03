#include <stdio.h>

typedef struct {
    int a;
    float b;
    double c;
} MixArg;

static long long score(MixArg value) {
    return (long long)value.a * 100LL +
           (long long)(value.b * 20.0f) +
           (long long)(value.c * 4.0);
}

int main(void) {
    MixArg value;
    value.a = 3;
    value.b = 1.5f;
    value.c = 2.25;
    printf("%lld\n", score(value));
    return 0;
}
