#include "04__probe_multitu_tag_function_parallel_namespace_shared.h"

int Wave14FuncTag(int seed) {
    return seed + 5;
}

int wave14_func_tag_read(void) {
    struct Wave14FuncTag item;
    item.value = 6;
    return item.value;
}
