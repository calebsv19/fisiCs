#include <stdarg.h>

struct wave52_var_pair {
    int lane[3];
};

typedef int (*wave52_var_score_fn)(struct wave52_var_pair pair, int salt);

struct wave52_var_box {
    struct wave52_var_pair pair;
    wave52_var_score_fn score;
    int bias;
};

int wave52_variadic_callback_payload(int seed, int count, ...) {
    va_list ap;
    int acc = seed + count;
    int i;

    va_start(ap, count);
    for (i = 0; i < count; i++) {
        struct wave52_var_box box = va_arg(ap, struct wave52_var_box);
        int scored = box.score(box.pair, box.bias + i);
        acc += scored * (i + 1) + box.pair.lane[i % 3] - box.bias;
    }
    va_end(ap);

    return acc;
}
