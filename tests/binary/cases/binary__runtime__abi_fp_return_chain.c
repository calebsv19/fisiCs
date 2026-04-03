#include <stdio.h>

static double step1(double x) {
    return x + 0.5;
}

static double step2(double x) {
    return x * 2.0;
}

static double step3(double x) {
    return x - 1.0;
}

int main(void) {
    double x = 2.25;
    double y = step3(step2(step1(x)));
    printf("%lld\n", (long long)(y * 8.0));
    return 0;
}
