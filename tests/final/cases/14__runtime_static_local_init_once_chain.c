#include <stdio.h>

static int wave80_chain_step(void) {
    static int seed = 7;
    static int calls = 0;
    calls += 1;
    seed += calls;
    return seed;
}

int main(void) {
    int a = wave80_chain_step();
    int b = wave80_chain_step();
    int c = wave80_chain_step();
    printf("%d %d %d\n", a, b, c);
    if (a != 8 || b != 10 || c != 13) {
        return 1;
    }
    return 0;
}
