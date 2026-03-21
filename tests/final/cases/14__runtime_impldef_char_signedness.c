#include <stdio.h>

int main(void) {
    volatile char c = (char)0xFF;

    int char_is_signed = ((char)-1 < 0);
    int promoted_c = c;

    printf("%d %d\n", char_is_signed, promoted_c);
    return 0;
}
