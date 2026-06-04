#include <setjmp.h>
#include <stdio.h>

static jmp_buf g_header_corpus_wave16_helper_env;
static volatile int g_header_corpus_wave16_helper_seen = 0;

static void header_corpus_wave16_helper_jump(int value) {
    g_header_corpus_wave16_helper_seen = value + 1;
    longjmp(g_header_corpus_wave16_helper_env, value);
}

int main(void) {
    int checkpoint = setjmp(g_header_corpus_wave16_helper_env);

    if (checkpoint == 0) {
        header_corpus_wave16_helper_jump(9);
    }

    printf(
        "helper=%d seen=%d\n",
        checkpoint,
        (int)g_header_corpus_wave16_helper_seen);
    return 0;
}
