#include <stdio.h>

#define W34_CURRENT_SLOT alpha
#define W34_CURRENT_VALUE 41
#include "03__probe_runtime_wave34_include_macro_state_current.h"

#line 2600 "virtual_wave34_include_current_main.c"
int main(void) {
    printf("%d %s %s %d\n", w34_current_alpha, W34_CURRENT_STR(W34_CURRENT_CAT(w34_current_, alpha)), __FILE__, __LINE__);
    return 0;
}
