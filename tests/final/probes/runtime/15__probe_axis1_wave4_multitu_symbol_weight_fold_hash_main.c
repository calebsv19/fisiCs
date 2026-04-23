#include <stdio.h>

extern unsigned axis1_wave4_fold(void);

int main(void) {
    unsigned folded = axis1_wave4_fold();
    printf("%u %u\n", folded, folded % 97u);
    return 0;
}
