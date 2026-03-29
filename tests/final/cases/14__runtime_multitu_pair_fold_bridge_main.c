#include <stdio.h>

struct Pair {
    int a;
    int b;
};

struct Pair step_pair(struct Pair p, int seed);
int pair_score(struct Pair p);

int main(void) {
    struct Pair p = {3, 5};
    int acc = 0;

    for (int i = 0; i < 7; ++i) {
        p = step_pair(p, i + 2);
        acc += pair_score(p);
    }

    printf("%d %d %d\n", acc, p.a, p.b);
    return 0;
}
