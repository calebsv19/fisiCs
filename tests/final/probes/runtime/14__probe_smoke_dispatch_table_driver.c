#include <stdio.h>

typedef int (*StepFn)(int state, int arg);

static int step_add(int state, int arg) { return state + arg + 1; }
static int step_mul(int state, int arg) { return state * (arg + 1); }
static int step_xor(int state, int arg) { return (state ^ (arg * 3 + 7)) + 2; }
static int step_mix(int state, int arg) { return (state + (arg * arg)) ^ (arg + 5); }

struct Step {
    unsigned op;
    int arg;
};

int main(void) {
    static StepFn table[] = {step_add, step_mul, step_xor, step_mix};
    struct Step program[] = {
        {0, 4},
        {1, 2},
        {3, 5},
        {2, 7},
        {1, 1},
        {0, 3}
    };

    int state = 5;
    int trace = 0;
    int n = (int)(sizeof(program) / sizeof(program[0]));
    int i;
    for (i = 0; i < n; ++i) {
        unsigned op = program[i].op % (unsigned)(sizeof(table) / sizeof(table[0]));
        state = table[op](state, program[i].arg);
        trace += state * (i + 3);
    }

    printf("%d %d\n", state, trace);
    return 0;
}
