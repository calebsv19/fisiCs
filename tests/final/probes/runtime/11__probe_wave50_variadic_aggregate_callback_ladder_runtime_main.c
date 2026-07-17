#include <stdio.h>

struct wave50_ladder_pair {
    int a;
    int b;
};

struct wave50_ladder_box {
    struct wave50_ladder_pair pair;
    int bonus;
};

typedef int (*wave50_ladder_score_fn)(struct wave50_ladder_pair item, int scale);

int wave50_variadic_aggregate_callback_ladder(wave50_ladder_score_fn score, int seed, int count, ...);

static int wave50_ladder_score(struct wave50_ladder_pair item, int scale) {
    return item.a * scale + item.b * 3;
}

int main(void) {
    struct wave50_ladder_box a = {{2, 5}, 4};
    struct wave50_ladder_box b = {{7, 1}, 3};
    struct wave50_ladder_box c = {{4, 6}, 2};
    printf("%d\n", wave50_variadic_aggregate_callback_ladder(wave50_ladder_score, 11, 3, a, b, c));
    return 0;
}
