#include "04__probe_multitu_include_tag_typedef_parallel_guard_shared.h"

extern int printf(const char*, ...);

int main(void) {
    struct Wave14Parallel item;
    item.value = 8;
    printf("%d %d\n", item.value, wave14_parallel_scalar_bias());
    return 0;
}
