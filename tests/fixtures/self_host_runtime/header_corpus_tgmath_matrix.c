#include <tgmath.h>
#include <stdio.h>

static int header_corpus_wave14_tgmath_summary(void) {
    double root = sqrt(81.0);
    double power = pow(3.0, 4.0);
    double distance = hypot(5.0, 12.0);
    double upper = fmax(-2.0, 7.0);
    double lower = fmin(-2.0, 7.0);
    double angle = atan2(3.0, 4.0);
    long angle1000 = (long)(angle * 1000.0 + 0.5);

    return (int)root * 1000 +
           (int)power * 10 +
           (int)distance +
           (int)upper +
           (int)lower +
           (int)angle1000;
}

int main(void) {
    int summary = header_corpus_wave14_tgmath_summary();

    if (summary != 9472) {
        return 1;
    }

    printf("summary=%d\n", summary);
    return 0;
}
