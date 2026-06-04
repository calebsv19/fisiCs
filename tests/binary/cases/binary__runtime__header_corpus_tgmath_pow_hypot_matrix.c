#include <tgmath.h>
#include <stdio.h>

int main(void) {
    double root = sqrt(81.0);
    double power = pow(3.0, 4.0);
    double distance = hypot(5.0, 12.0);
    double upper = fmax(-2.0, 7.0);
    double lower = fmin(-2.0, 7.0);

    if (root != 9.0) {
        return 1;
    }
    if (power != 81.0) {
        return 2;
    }
    if (distance != 13.0) {
        return 3;
    }
    if (upper != 7.0) {
        return 4;
    }
    if (lower != -2.0) {
        return 5;
    }

    printf(
        "root=%.0f power=%.0f distance=%.0f upper=%.0f lower=%.0f\n",
        root,
        power,
        distance,
        upper,
        lower);
    return 0;
}
