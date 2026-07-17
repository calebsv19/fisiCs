#ifndef FISICS_PROBE_RUNTIME_WAVE35_LINE_STACK_REENTRY_HELPERS_H
#define FISICS_PROBE_RUNTIME_WAVE35_LINE_STACK_REENTRY_HELPERS_H

#define W35_STACK_CAT2(a, b) a##b
#define W35_STACK_CAT(a, b) W35_STACK_CAT2(a, b)
#define W35_STACK_DECL(slot, value, line_value) enum { W35_STACK_CAT(w35_stack_, slot) = (value) + (line_value) }

#endif

#line 3700 "virtual_wave35_stack_header.h"
W35_STACK_DECL(W35_STACK_SLOT, W35_STACK_VALUE, __LINE__);
