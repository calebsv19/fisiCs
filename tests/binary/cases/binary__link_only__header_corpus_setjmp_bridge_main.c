#include <setjmp.h>

int header_corpus_wave16_bridge_result(int seed);

int main(void) {
    jmp_buf env;
    int checkpoint = setjmp(env);

    if (checkpoint != 0) {
        return checkpoint;
    }

    return header_corpus_wave16_bridge_result(5) == 8 ? 0 : 1;
}
