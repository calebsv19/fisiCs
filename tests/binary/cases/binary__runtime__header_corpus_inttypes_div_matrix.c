#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    static const uint8_t bytes[] = {3U, 1U, 4U, 1U};
    imaxdiv_t split = imaxdiv(INTMAX_C(29), INTMAX_C(5));
    intmax_t magnitude = imaxabs(INTMAX_C(-17));
    uintptr_t span = (uintptr_t)(&bytes[3] - &bytes[0]);
    bool ok = split.quot == INTMAX_C(5) &&
              split.rem == INTMAX_C(4) &&
              magnitude == INTMAX_C(17) &&
              span == (uintptr_t)3;

    printf(
        "quot=%" PRIdMAX " rem=%" PRIdMAX " abs=%" PRIdMAX " span=%" PRIuPTR " ok=%d\n",
        split.quot,
        split.rem,
        magnitude,
        span,
        ok ? 1 : 0);
    return ok ? 0 : 1;
}
