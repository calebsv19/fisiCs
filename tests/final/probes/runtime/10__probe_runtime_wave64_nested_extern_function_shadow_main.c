#include <stdio.h>

int bucket10_wave64_nested_liba(int step);
int bucket10_wave64_nested_libb(int step);

static int bucket10_wave64_nested_main(int step) {
    extern int bucket10_wave64_nested_external(int);
    int first = bucket10_wave64_nested_external(step);
    {
        int bucket10_wave64_nested_external = first - step;
        return bucket10_wave64_nested_external + first;
    }
}

int main(void) {
    extern int bucket10_wave64_nested_external(int);

    printf("%d %d %d %d\n",
           bucket10_wave64_nested_liba(3),
           bucket10_wave64_nested_main(2),
           bucket10_wave64_nested_libb(4),
           bucket10_wave64_nested_external(1));
    return 0;
}
