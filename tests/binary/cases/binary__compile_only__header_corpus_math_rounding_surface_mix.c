#include <math.h>

double wave30_math_rounding_surface(double value) {
    return floor(value) + ceil(value) + trunc(value) + round(value) + fabs(value);
}

int main(void) {
    return 0;
}
