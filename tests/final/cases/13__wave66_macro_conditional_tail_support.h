#ifndef FISICS_13_WAVE66_MACRO_CONDITIONAL_TAIL_SUPPORT_H
#define FISICS_13_WAVE66_MACRO_CONDITIONAL_TAIL_SUPPORT_H

#line 6610 "virtual_wave66_macro_conditional_tail_support.h"
#ifdef WAVE66_REPAIRED
#define WAVE66_LEAF wave66_present_value
#else
#define WAVE66_LEAF wave66_missing_value
#endif

#define WAVE66_CONDITIONAL(flag) ((flag) ? WAVE66_LEAF : 66)
#define WAVE66_ASSIGN_TAIL(flag, slot) \
    ((slot) = WAVE66_CONDITIONAL(flag))

#endif
