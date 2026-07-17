#include <stdio.h>

#define W35_STATE_DIAG_ARM 0
#define W35_STATE_SLOT alpha
#define W35_STATE_VALUE 13
#include "03__probe_runtime_wave35_include_state_reentry_diag_boundary.h"

#undef W35_STATE_DIAG_ARM
#undef W35_STATE_SLOT
#undef W35_STATE_VALUE
#define W35_STATE_DIAG_ARM 0
#define W35_STATE_SLOT beta
#define W35_STATE_VALUE 21
#include "03__probe_runtime_wave35_include_state_reentry_diag_boundary.h"

#line 3600 "virtual_wave35_state_main.c"
int main(void) {
    printf("%d %d %s %s %d\n", w35_state_alpha, w35_state_beta, W35_STATE_STR(W35_STATE_CAT(w35_state_, beta)), __FILE__, __LINE__);
    return 0;
}
