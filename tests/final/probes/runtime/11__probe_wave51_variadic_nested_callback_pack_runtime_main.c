#include <stdio.h>

struct wave51_var_pair {
    int a;
    int b;
};

typedef int (*wave51_var_score_fn)(struct wave51_var_pair pair, int salt);

struct wave51_var_box {
    struct wave51_var_pair pair;
    wave51_var_score_fn score;
    int salt;
};

int wave51_variadic_nested_callback_pack(int seed, int count, ...);

static int wave51_score_sum(struct wave51_var_pair pair, int salt) {
    return pair.a * 2 + pair.b + salt;
}

static int wave51_score_delta(struct wave51_var_pair pair, int salt) {
    return pair.a * salt - pair.b * 3;
}

int main(void) {
    struct wave51_var_box a = {{2, 9}, wave51_score_sum, 4};
    struct wave51_var_box b = {{5, 3}, wave51_score_delta, 6};
    struct wave51_var_box c = {{7, 1}, wave51_score_sum, 2};
    printf("%d\n", wave51_variadic_nested_callback_pack(13, 3, a, b, c));
    return 0;
}
