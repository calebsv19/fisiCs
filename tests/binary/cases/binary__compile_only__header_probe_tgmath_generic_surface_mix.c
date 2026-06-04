#include <tgmath.h>

int main(void) {
    double real_value = cos(0.0);
    double abs_value = fabs(-3.5);
    double root_value = sqrt(16.0);
    return (real_value == 1.0 && abs_value == 3.5 && root_value == 4.0) ? 0 : 1;
}
