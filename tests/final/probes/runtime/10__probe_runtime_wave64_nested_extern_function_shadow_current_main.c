#include <stdio.h>

int bucket10_wave64_nested_current_external(int value);
int bucket10_wave64_nested_current_liba(int step);
int bucket10_wave64_nested_current_libb(int step);

static int bucket10_wave64_nested_current_main(int step) {
    int first = bucket10_wave64_nested_current_external(step);
    {
        int bucket10_wave64_nested_current_external = first - step;
        return bucket10_wave64_nested_current_external + first;
    }
}

int main(void) {
    printf("%d %d %d %d\n",
           bucket10_wave64_nested_current_liba(3),
           bucket10_wave64_nested_current_main(2),
           bucket10_wave64_nested_current_libb(4),
           bucket10_wave64_nested_current_external(1));
    return 0;
}
