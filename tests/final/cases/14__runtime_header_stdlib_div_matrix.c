#include <stdio.h>
#include <stdlib.h>

int main(void) {
    div_t first = div(29, 5);
    ldiv_t second = ldiv(-17L, 4L);

    if (first.quot != 5 || first.rem != 4) {
        return 1;
    }
    if (second.quot != -4L || second.rem != -1L) {
        return 2;
    }

    printf(
        "div=%d/%d ldiv=%ld/%ld abs=%d labs=%ld\n",
        first.quot,
        first.rem,
        second.quot,
        second.rem,
        abs(-17),
        labs(-17L));
    return 0;
}
