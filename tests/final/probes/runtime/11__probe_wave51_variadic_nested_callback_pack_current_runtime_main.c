#include <stdio.h>

struct wave51_var_current_pair {
    int a;
    int b;
};

struct wave51_var_current_box {
    struct wave51_var_current_pair pair;
    int salt;
};

typedef int (*wave51_var_current_score_fn)(struct wave51_var_current_pair pair, int salt);

int wave51_variadic_nested_callback_pack_current(wave51_var_current_score_fn score, int seed, int count, ...);

static int wave51_current_score(struct wave51_var_current_pair pair, int salt) {
    return pair.a * 2 + pair.b + salt;
}

int main(void) {
    struct wave51_var_current_box a = {{2, 9}, 4};
    struct wave51_var_current_box b = {{5, 3}, 6};
    struct wave51_var_current_box c = {{7, 1}, 2};
    printf("%d\n", wave51_variadic_nested_callback_pack_current(wave51_current_score, 13, 3, a, b, c));
    return 0;
}
