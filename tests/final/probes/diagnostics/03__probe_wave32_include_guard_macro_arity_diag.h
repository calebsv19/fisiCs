#ifndef FISICS_PROBE_WAVE32_INCLUDE_GUARD_MACRO_ARITY_DIAG_H
#define FISICS_PROBE_WAVE32_INCLUDE_GUARD_MACRO_ARITY_DIAG_H

#define W32_ARITY_PAIR(a, b) ((a) + (b))
#define W32_ARITY_WRAP(x) W32_ARITY_PAIR(x)
#define W32_ARITY_STR2(x) #x
#define W32_ARITY_STR(x) W32_ARITY_STR2(x)

#if 0
#define W32_ARITY_PAIR(a, b, c) inactive_branch_must_not_replace_the_live_pair
int w32_inactive_noise = W32_ARITY_PAIR(1);
#endif

#line 2040 "virtual_wave32_arity_header.h"
const char *w32_arity_label = W32_ARITY_STR(W32_ARITY_PAIR(left, right));
int w32_header_arity_error = W32_ARITY_WRAP(5);

#endif
