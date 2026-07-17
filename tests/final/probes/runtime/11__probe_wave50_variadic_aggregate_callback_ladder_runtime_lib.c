#include <stdarg.h>

struct wave50_ladder_pair {
    int a;
    int b;
};

struct wave50_ladder_box {
    struct wave50_ladder_pair pair;
    int bonus;
};

typedef int (*wave50_ladder_score_fn)(struct wave50_ladder_pair item, int scale);

int wave50_variadic_aggregate_callback_ladder(wave50_ladder_score_fn score, int seed, int count, ...) {
    va_list ap;
    int acc = seed + count;
    int i;

    va_start(ap, count);
    for (i = 0; i < count; i++) {
        struct wave50_ladder_box box = va_arg(ap, struct wave50_ladder_box);
        int local = score(box.pair, box.bonus + i);
        acc += local * (i + 2) + box.pair.a - box.pair.b;
    }
    va_end(ap);

    return acc;
}
