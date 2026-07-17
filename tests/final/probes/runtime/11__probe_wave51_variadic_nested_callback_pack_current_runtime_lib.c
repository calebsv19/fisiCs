#include <stdarg.h>

struct wave51_var_current_pair {
    int a;
    int b;
};

struct wave51_var_current_box {
    struct wave51_var_current_pair pair;
    int salt;
};

typedef int (*wave51_var_current_score_fn)(struct wave51_var_current_pair pair, int salt);

int wave51_variadic_nested_callback_pack_current(wave51_var_current_score_fn score, int seed, int count, ...) {
    va_list ap;
    int acc = seed + count;
    int i;

    va_start(ap, count);
    for (i = 0; i < count; i++) {
        struct wave51_var_current_box box = va_arg(ap, struct wave51_var_current_box);
        int scored = score(box.pair, box.salt + i);
        acc += scored * (i + 2) + box.pair.a - box.pair.b + box.salt;
    }
    va_end(ap);

    return acc;
}
