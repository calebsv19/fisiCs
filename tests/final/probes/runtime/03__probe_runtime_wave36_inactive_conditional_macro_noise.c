#include <stdio.h>

#define W36_NOISE_ARM 0
#define W36_NOISE_SLOT stable
#define W36_NOISE_VALUE 64
#include "03__probe_runtime_wave36_inactive_conditional_macro_noise.h"

#line 4450 "virtual_wave36_inactive_noise_main.c"
int main(void) {
    printf("%d %s %s %d\n", w36_noise_stable, W36_NOISE_STR(W36_NOISE_CAT(w36_noise_, stable)), __FILE__, __LINE__);
    return 0;
}
