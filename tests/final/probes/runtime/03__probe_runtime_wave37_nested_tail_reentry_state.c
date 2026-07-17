#include <stdio.h>

#define W37_NEST_SLOT west
#define W37_NEST_VALUE 23
#include "03__probe_runtime_wave37_nested_tail_reentry_state.h"

#undef W37_NEST_SLOT
#undef W37_NEST_VALUE
#define W37_NEST_SLOT east
#define W37_NEST_VALUE 41
#include "03__probe_runtime_wave37_nested_tail_reentry_state.h"

#line 5200 "virtual_wave37_nested_tail_main.c"
int main(void) {
    printf("%d %d %d %s %s %d\n", w37_nested_west, w37_nested_east, W37_NEST_SEED, W37_NEST_STR(W37_NEST_CAT(w37_nested_, east)), __FILE__, __LINE__);
    return 0;
}
