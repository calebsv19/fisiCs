#include <stdio.h>

#define W37_NOISE_CAT2(a, b) a##b
#define W37_NOISE_CAT(a, b) W37_NOISE_CAT2(a, b)
#define W37_NOISE_STR2(x) #x
#define W37_NOISE_STR(x) W37_NOISE_STR2(x)
#define W37_NOISE_DECL(slot, value) enum { W37_NOISE_CAT(w37_noise_, slot) = (value) }

#if 0
#define W37_NOISE_SLOT broken
#define W37_NOISE_VALUE 808
#include "03__probe_runtime_wave37_inactive_noise_missing.h"
#error wave37 inactive provenance noise should stay skipped
#endif

#define W37_NOISE_SLOT stable
#line 5300 "virtual_wave37_inactive_noise_header.h"
W37_NOISE_DECL(W37_NOISE_SLOT, 73);

#line 5350 "virtual_wave37_inactive_noise_main.c"
int main(void) {
    printf("%d %s %s %d\n", w37_noise_stable, W37_NOISE_STR(W37_NOISE_CAT(w37_noise_, stable)), __FILE__, __LINE__);
    return 0;
}
