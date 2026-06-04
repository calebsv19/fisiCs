#include <setjmp.h>

static jmp_buf g_header_corpus_wave16_control_env;

static int header_corpus_wave16_control_surface(int seed) {
    volatile int total = seed;
    int checkpoint = setjmp(g_header_corpus_wave16_control_env);

    if (checkpoint == 0) {
        total += 1;
    } else if (checkpoint > 0) {
        total += checkpoint;
    } else {
        total -= checkpoint;
    }

    return total;
}

int main(void) {
    return header_corpus_wave16_control_surface(2) == 3 ? 0 : 1;
}
