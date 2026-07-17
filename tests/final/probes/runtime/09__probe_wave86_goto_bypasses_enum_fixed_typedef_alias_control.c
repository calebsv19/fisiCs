#include <stdio.h>

int main(void) {
    goto wave86_fixed_done;

    enum { WAVE86_FIXED_BOUND = 4 };
    typedef int Wave86FixedRow[WAVE86_FIXED_BOUND];
    typedef Wave86FixedRow Wave86FixedAlias;

wave86_fixed_done:
    ;
    Wave86FixedAlias wave86_values = {3, 5, 7, 11};
    int wave86_sum = wave86_values[0] +
                     wave86_values[1] +
                     wave86_values[2] +
                     wave86_values[3];

    printf("%d %zu\n", wave86_sum, sizeof(Wave86FixedAlias));
    return !(wave86_sum == 26 &&
             sizeof(Wave86FixedAlias) ==
                 WAVE86_FIXED_BOUND * sizeof(int));
}
