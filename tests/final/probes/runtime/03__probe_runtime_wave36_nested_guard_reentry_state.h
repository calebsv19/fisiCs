#ifndef FISICS_PROBE_RUNTIME_WAVE36_NESTED_GUARD_REENTRY_STATE_H
#define FISICS_PROBE_RUNTIME_WAVE36_NESTED_GUARD_REENTRY_STATE_H

#include "03__probe_runtime_wave36_nested_guard_reentry_state_inner.h"

#endif

#if W36_NEST_INACTIVE_ARM
#define W36_NEST_VALUE 900
#error wave36 nested guard inactive branch should stay skipped
#endif

#line 4050 "virtual_wave36_nested_guard_header.h"
W36_NEST_DECL(W36_NEST_SLOT, W36_NEST_VALUE);
