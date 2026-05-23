#include <stdio.h>

extern unsigned pointer_lifetime_window_feedback(unsigned seed);

int main(void) {
    printf("%u %u\n",
           pointer_lifetime_window_feedback(61u),
           pointer_lifetime_window_feedback(173u));
    return 0;
}
