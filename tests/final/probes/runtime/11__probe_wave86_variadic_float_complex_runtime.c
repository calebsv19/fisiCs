#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void wave86_capture(int marker, ...) {
    va_list args;
    float _Complex complex_value;
    double promoted_real;
    float complex_lanes[2];

    va_start(args, marker);
    complex_value = va_arg(args, float _Complex);
    promoted_real = va_arg(args, double);
    va_end(args);

    memcpy(complex_lanes, &complex_value, sizeof(complex_value));
    printf("%d %d %d\n",
           (int)(complex_lanes[0] * 4.0f),
           (int)(complex_lanes[1] * 4.0f),
           (int)(promoted_real * 4.0));
}

int main(void) {
    float source_lanes[2] = {1.25f, -2.5f};
    float _Complex complex_value;
    float real_value = 3.75f;

    memcpy(&complex_value, source_lanes, sizeof(complex_value));
    wave86_capture(1, complex_value, real_value);
    return 0;
}
