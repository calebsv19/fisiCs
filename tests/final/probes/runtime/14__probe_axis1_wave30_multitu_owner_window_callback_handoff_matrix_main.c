#include <stdio.h>

extern unsigned axis1_wave30_owner_window_callback_handoff_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave30_owner_window_callback_handoff_matrix(57u),
           axis1_wave30_owner_window_callback_handoff_matrix(104u));
    return 0;
}
