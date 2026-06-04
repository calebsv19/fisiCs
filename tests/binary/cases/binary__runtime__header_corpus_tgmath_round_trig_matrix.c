#include <tgmath.h>
#include <stdio.h>

int main(void) {
    double raised = ceil(2.25);
    double lowered = floor(-2.25);
    double magnitude = fabs(-4.5);
    double angle = atan2(3.0, 4.0);
    double cosine = cos(0.0);
    double sine = sin(0.0);
    long angle1000 = (long)(angle * 1000.0 + 0.5);
    long magnitude100 = (long)(magnitude * 100.0 + 0.5);
    long cosine100 = (long)(cosine * 100.0 + 0.5);
    long sine100 = (long)(sine * 100.0 + 0.5);

    if (raised != 3.0) {
        return 1;
    }
    if (lowered != -3.0) {
        return 2;
    }
    if (magnitude100 != 450L) {
        return 3;
    }
    if (angle1000 != 644L) {
        return 4;
    }
    if (cosine100 != 100L) {
        return 5;
    }
    if (sine100 != 0L) {
        return 6;
    }

    printf(
        "ceil=%.0f floor=%.0f abs100=%ld angle1000=%ld cos100=%ld sin100=%ld\n",
        raised,
        lowered,
        magnitude100,
        angle1000,
        cosine100,
        sine100);
    return 0;
}
