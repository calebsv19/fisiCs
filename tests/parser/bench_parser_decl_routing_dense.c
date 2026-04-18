typedef int T0;
typedef int T1;
typedef int T2;
typedef int T3;
typedef int T4;
typedef int T5;
typedef int T6;
typedef int T7;

typedef struct Pair {
    int left;
    int right;
} Pair;

static T0 g0 = 0;
static T1 g1 = 1;
static T2 g2 = 2;
static T3 g3 = 3;

static int helper_add(T4 x, T5 y);
static int helper_mix(T6 x, T7 y);

static int helper_add(T4 x, T5 y) {
    return x + y;
}

static int helper_mix(T6 x, T7 y) {
    return x - y;
}

static int bench_decl_routing_dense(int seed) {
    int acc = seed + g0 + g1 + g2 + g3;

    {
        T0 a = acc;
        T1 b = a + 1;
        Pair p = { a, b };
        acc += p.left + p.right;
    }

    {
        T2 c = acc + 2;
        T3 d = c + 3;
        Pair p = { c, d };
        acc += p.left;
        acc += p.right;
    }

    for (T4 i = 0; i < 3; ++i) {
        T5 local = helper_add(i, acc);
        acc += local;
    }

    for (T6 j = 0; j < 2; ++j) {
        T7 local = helper_mix(acc, j);
        acc += local;
    }

    return acc;
}

int main(void) {
    int total = 0;
    total += bench_decl_routing_dense(1);
    total += bench_decl_routing_dense(2);
    total += bench_decl_routing_dense(3);
    return total == 0;
}
