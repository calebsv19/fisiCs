#include <stdio.h>
#include <tgmath.h>

int main(void) {
    float rf = sqrt(9.0f);
    double rd = sqrt(16.0);
    long double rl = sqrt(25.0L);
    float af = fabs(-7.25f);
    long double cl = ceil(2.1L);
    long rf100 = (long)(rf * 100.0f + 0.5f);
    long rd100 = (long)(rd * 100.0 + 0.5);
    long rl100 = (long)(rl * 100.0L + 0.5L);
    long af100 = (long)(af * 100.0f + 0.5f);
    long cl100 = (long)(cl * 100.0L + 0.5L);
    long summary = rf100 + rd100 + rl100 + af100 + cl100;

    printf("tgmath-real sqrt=%ld/%ld/%ld abs=%ld ceil=%ld summary=%ld\n",
           rf100,
           rd100,
           rl100,
           af100,
           cl100,
           summary);

    return rf100 == 300L && rd100 == 400L && rl100 == 500L && af100 == 725L &&
                   cl100 == 300L && summary == 2225L
               ? 0
               : 1;
}
