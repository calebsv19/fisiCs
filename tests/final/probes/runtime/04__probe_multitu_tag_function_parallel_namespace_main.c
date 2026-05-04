#include "04__probe_multitu_tag_function_parallel_namespace_shared.h"

extern int printf(const char*, ...);

int main(void) {
    struct Wave14FuncTag item;
    item.value = wave14_func_tag_read();
    printf("%d %d\n", Wave14FuncTag(4), item.value);
    return 0;
}
