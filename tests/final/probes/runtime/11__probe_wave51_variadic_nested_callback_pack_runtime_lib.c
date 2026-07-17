#include <stdarg.h>

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

int wave51_variadic_nested_callback_pack(int seed, int count, ...) {
    va_list ap;
    int acc = seed + count;
    int i;

    va_start(ap, count);
    for (i = 0; i < count; i++) {
        struct wave51_var_box box = va_arg(ap, struct wave51_var_box);
        int scored = box.score(box.pair, box.salt + i);
        acc += scored * (i + 2) + box.pair.a - box.pair.b + box.salt;
    }
    va_end(ap);

    return acc;
}
