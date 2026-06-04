#include <setjmp.h>

int header_corpus_wave16_bridge_result(int seed) {
    jmp_buf env;
    volatile int result = seed;
    int checkpoint = setjmp(env);

    if (checkpoint == 0) {
        result += 3;
    } else {
        result += checkpoint;
    }

    return result;
}
