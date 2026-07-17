#include <stdio.h>

int bucket10_wave65_nested_dispatch(int value);
int bucket10_wave65_nested_liba(int step);
int bucket10_wave65_nested_libb(int step);

static int bucket10_wave65_nested_main(int step) {
    {
        extern int bucket10_wave65_nested_dispatch(int value);
        int first = bucket10_wave65_nested_dispatch(step);
        {
            int bucket10_wave65_nested_dispatch = first - step;
            return bucket10_wave65_nested_dispatch + first;
        }
    }
}

int main(void) {
    printf("%d %d %d %d\n",
           bucket10_wave65_nested_liba(4),
           bucket10_wave65_nested_main(3),
           bucket10_wave65_nested_libb(2),
           bucket10_wave65_nested_dispatch(1));
    return 0;
}
