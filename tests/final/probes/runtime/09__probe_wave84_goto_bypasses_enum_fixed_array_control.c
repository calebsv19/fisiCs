#include <stdio.h>

int main(void) {
    int wave84_marker = 11;

    goto wave84_after_array;

    enum { WAVE84_BOUND = 3 };
    int wave84_values[WAVE84_BOUND];

wave84_after_array:
    wave84_values[0] = wave84_marker;
    wave84_values[1] = WAVE84_BOUND + 4;
    wave84_values[2] = (int)sizeof(wave84_values);

    printf("%d %d %d %zu\n",
           wave84_values[0],
           wave84_values[1],
           wave84_values[2],
           sizeof(wave84_values) / sizeof(wave84_values[0]));

    return !(
        wave84_values[0] == 11 &&
        wave84_values[1] == 7 &&
        wave84_values[2] == (int)(3 * sizeof(int)) &&
        sizeof(wave84_values) / sizeof(wave84_values[0]) == 3
    );
}
