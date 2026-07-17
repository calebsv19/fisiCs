#include <stdio.h>

#define W34_REENTRY_SLOT alpha
#define W34_REENTRY_VALUE 41
#include "03__probe_runtime_wave34_include_reentry_slot_rebind.h"

#undef W34_REENTRY_SLOT
#undef W34_REENTRY_VALUE
#define W34_REENTRY_SLOT beta
#define W34_REENTRY_VALUE 64
#include "03__probe_runtime_wave34_include_reentry_slot_rebind.h"

#line 2600 "virtual_wave34_reentry_main.c"
int main(void) {
    printf("%d %d %s %s %d\n", w34_reentry_alpha, w34_reentry_beta, W34_REENTRY_STR(W34_REENTRY_CAT(w34_reentry_, alpha)), __FILE__, __LINE__);
    return 0;
}
