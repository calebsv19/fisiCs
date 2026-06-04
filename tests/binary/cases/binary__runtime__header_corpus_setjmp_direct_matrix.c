#include <setjmp.h>
#include <stdio.h>

static jmp_buf g_header_corpus_wave16_direct_env;
static volatile int g_header_corpus_wave16_direct_seen = 0;

int main(void) {
    int checkpoint = setjmp(g_header_corpus_wave16_direct_env);

    if (checkpoint == 0) {
        g_header_corpus_wave16_direct_seen = 1;
        longjmp(g_header_corpus_wave16_direct_env, 7);
    }

    printf(
        "direct=%d seen=%d\n",
        checkpoint,
        (int)g_header_corpus_wave16_direct_seen);
    return 0;
}
