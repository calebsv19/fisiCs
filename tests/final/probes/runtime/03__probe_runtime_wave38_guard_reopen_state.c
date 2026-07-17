#include <stdio.h>

#define W38_REOPEN_SLOT alpha
#define W38_REOPEN_VALUE 12
#include "03__probe_runtime_wave38_guard_reopen_state.h"

#undef FISICS_PROBE_RUNTIME_WAVE38_GUARD_REOPEN_STATE_H
#undef W38_REOPEN_SLOT
#undef W38_REOPEN_VALUE
#define W38_REOPEN_SLOT beta
#define W38_REOPEN_VALUE 29
#include "03__probe_runtime_wave38_guard_reopen_state.h"

int main(void) {
    printf("%d %d\n", w38_reopen_alpha, w38_reopen_beta);
    return 0;
}
