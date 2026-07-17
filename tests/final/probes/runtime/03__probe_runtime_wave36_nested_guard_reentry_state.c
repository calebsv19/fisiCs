#include <stdio.h>

#define W36_NEST_INACTIVE_ARM 0
#define W36_NEST_SLOT red
#define W36_NEST_VALUE 17
#include "03__probe_runtime_wave36_nested_guard_reentry_state.h"

#undef W36_NEST_SLOT
#undef W36_NEST_VALUE
#define W36_NEST_SLOT blue
#define W36_NEST_VALUE 29
#include "03__probe_runtime_wave36_nested_guard_reentry_state.h"

#line 4100 "virtual_wave36_nested_guard_main.c"
int main(void) {
    printf("%d %d %d %s %s %d\n", w36_nested_red, w36_nested_blue, W36_NEST_HELPER_SEED, W36_NEST_STR(W36_NEST_CAT(w36_nested_, blue)), __FILE__, __LINE__);
    return 0;
}
