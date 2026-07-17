#include <stdio.h>

struct wave52_var_pair {
    int lane[3];
};

typedef int (*wave52_var_score_fn)(struct wave52_var_pair pair, int salt);

struct wave52_var_box {
    struct wave52_var_pair pair;
    wave52_var_score_fn score;
    int bias;
};

int wave52_variadic_callback_payload(int seed, int count, ...);

static int wave52_score_pair(struct wave52_var_pair pair, int salt) {
    return pair.lane[0] * 3 + pair.lane[1] * 5 - pair.lane[2] + salt;
}

int main(void) {
    struct wave52_var_box a = {{{2, 4, 1}}, wave52_score_pair, 3};
    struct wave52_var_box b = {{{5, 1, 6}}, wave52_score_pair, 2};
    struct wave52_var_box c = {{{3, 7, 2}}, wave52_score_pair, 5};
    printf("%d\n", wave52_variadic_callback_payload(11, 3, a, b, c));
    return 0;
}
