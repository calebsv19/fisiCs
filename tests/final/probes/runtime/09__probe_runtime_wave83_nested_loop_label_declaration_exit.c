#include <stdio.h>

int main(void) {
    int outer;
    int total = 0;

    for (outer = 1; outer <= 4; ++outer) {
        int inner;
        int base = outer * 10;

        for (inner = 0; inner < 4; ++inner) {
            int local = base + inner;

            total += local;
            if (inner == outer - 1) {
                goto outer_tail;
            }
            total += 1;
        }

outer_tail:
        total += base + outer;
    }

    printf("%d\n", total);
    return 0;
}
