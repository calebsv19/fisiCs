#ifndef FISICS_PROBE_RUNTIME_WAVE35_INCLUDE_STATE_REENTRY_HELPERS_H
#define FISICS_PROBE_RUNTIME_WAVE35_INCLUDE_STATE_REENTRY_HELPERS_H

#define W35_STATE_CAT2(a, b) a##b
#define W35_STATE_CAT(a, b) W35_STATE_CAT2(a, b)
#define W35_STATE_STR2(x) #x
#define W35_STATE_STR(x) W35_STATE_STR2(x)
#define W35_STATE_DECL(slot, value) enum { W35_STATE_CAT(w35_state_, slot) = value }

#endif

#if W35_STATE_DIAG_ARM
#error wave35 inactive diagnostic boundary should stay skipped
#endif

#line 3500 "virtual_wave35_state_header.h"
W35_STATE_DECL(W35_STATE_SLOT, W35_STATE_VALUE);
