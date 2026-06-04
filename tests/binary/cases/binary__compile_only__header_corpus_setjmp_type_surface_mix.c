#include <setjmp.h>
#include <stddef.h>

static jmp_buf g_header_corpus_wave16_env;

static int header_corpus_wave16_type_surface(jmp_buf env, int seed) {
    volatile int marker = seed;
    int checkpoint = setjmp(env);

    if (checkpoint == 0) {
        marker += 3;
    } else {
        marker += checkpoint;
    }

    return (int)sizeof(g_header_corpus_wave16_env) + marker;
}

int main(void) {
    return header_corpus_wave16_type_surface(g_header_corpus_wave16_env, 4) > 0 ? 0 : 1;
}
