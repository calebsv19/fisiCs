#include "13__wave65_macro_cascade_support.h"

#line 6550 "virtual_wave65_macro_include_cascade.c"
int wave65_prefix_function(void) {
    return WAVE65_EXPRESSION();
}

int wave65_tail_global = 655;

int wave65_tail_function(void) {
    return wave65_tail_global + 1;
}
