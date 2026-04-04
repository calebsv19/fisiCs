#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* raw_a = "6.25e1tail";
    const char* raw_b = "-0.125xyz";
    char* end_a = 0;
    char* end_b = 0;
    double a = strtod(raw_a, &end_a);
    double b = strtod(raw_b, &end_b);
    long a2;
    long b8;
    int c_a;
    int c_b;

    if (!end_a || !end_b) {
        return 1;
    }
    if (a < 62.49 || a > 62.51 || b < -0.126 || b > -0.124) {
        return 2;
    }

    c_a = (int)(end_a - raw_a);
    c_b = (int)(end_b - raw_b);
    if (c_a != 6 || c_b != 6) {
        return 3;
    }

    a2 = (long)(a * 2.0);
    b8 = (long)(b * 8.0);
    if (a2 != 125L || b8 != -1L) {
        return 4;
    }

    printf("strtod_endptr_matrix_ok a2=%ld b8=%ld consumed=%d/%d\n", a2, b8, c_a, c_b);
    return 0;
}
