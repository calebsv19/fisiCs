#include <stdio.h>

#define W32_NOISE_CAT2(a, b) a##b
#define W32_NOISE_CAT(a, b) W32_NOISE_CAT2(a, b)
#define W32_NOISE_STR2(x) #x
#define W32_NOISE_STR(x) W32_NOISE_STR2(x)

#if 0
#include "03__probe_runtime_wave32_missing_inactive_header.h"
#define W32_NOISE_PICK(a, b, c) inactive branch should not define the active macro
#define W32_NOISE_NAME inactive_token
#endif

#line 700 "virtual_wave32_inactive_noise.c"
#define W32_NOISE_PICK(a, b, c) ((a) * 100 + (b) * 10 + (c))
#define W32_NOISE_LABEL(x) W32_NOISE_STR(W32_NOISE_CAT(w32_noise_, x))

int main(void) {
    printf("%d %s %d\n", W32_NOISE_PICK(4, 5, 6), W32_NOISE_LABEL(clean), __LINE__);
    return 0;
}
