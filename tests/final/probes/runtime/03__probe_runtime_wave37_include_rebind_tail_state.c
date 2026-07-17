#include <stdio.h>

#define W37_REBIND_SLOT alpha
#define W37_REBIND_VALUE 19
#include "03__probe_runtime_wave37_include_rebind_tail_state.h"

#undef W37_REBIND_SLOT
#undef W37_REBIND_VALUE
#undef W37_REBIND_OFFSET
#define W37_REBIND_OFFSET 6
#define W37_REBIND_SLOT beta
#define W37_REBIND_VALUE 31
#include "03__probe_runtime_wave37_include_rebind_tail_state.h"

#line 5100 "virtual_wave37_rebind_tail_main.c"
int main(void) {
    printf("%d %d %d %s %s %d\n", w37_rebind_alpha, w37_rebind_beta, W37_REBIND_OFFSET, W37_REBIND_STR(W37_REBIND_CAT(w37_rebind_, beta)), __FILE__, __LINE__);
    return 0;
}
