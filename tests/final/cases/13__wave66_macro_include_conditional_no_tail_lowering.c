#include "13__wave66_macro_conditional_tail_support.h"

#line 6650 "virtual_wave66_macro_include_conditional.c"
int wave66_prefix_function(int wave66_flag) {
    int wave66_slot = 0;
    return WAVE66_ASSIGN_TAIL(wave66_flag, wave66_slot);
}

int wave66_tail_global = 666;

int wave66_tail_function(void) {
    return wave66_tail_global + 1;
}
