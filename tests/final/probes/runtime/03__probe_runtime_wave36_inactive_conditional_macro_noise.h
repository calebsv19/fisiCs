#ifndef FISICS_PROBE_RUNTIME_WAVE36_INACTIVE_CONDITIONAL_MACRO_NOISE_H
#define FISICS_PROBE_RUNTIME_WAVE36_INACTIVE_CONDITIONAL_MACRO_NOISE_H

#define W36_NOISE_CAT2(a, b) a##b
#define W36_NOISE_CAT(a, b) W36_NOISE_CAT2(a, b)
#define W36_NOISE_STR2(x) #x
#define W36_NOISE_STR(x) W36_NOISE_STR2(x)
#define W36_NOISE_DECL(slot, value) enum { W36_NOISE_CAT(w36_noise_, slot) = (value) }

#endif

#if W36_NOISE_ARM
#undef W36_NOISE_VALUE
#define W36_NOISE_VALUE 999
#include "03__probe_runtime_wave36_inactive_missing_noise.h"
#error wave36 inactive macro noise should stay skipped
#endif

#line 4400 "virtual_wave36_inactive_noise_header.h"
W36_NOISE_DECL(W36_NOISE_SLOT, W36_NOISE_VALUE);
