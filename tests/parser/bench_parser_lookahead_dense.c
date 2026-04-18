typedef int T0;
typedef int T1;
typedef int T2;
typedef int T3;
typedef int T4;
typedef int T5;
typedef int T6;
typedef int T7;
typedef int T8;
typedef int T9;
typedef int T10;
typedef int T11;
typedef int T12;
typedef int T13;
typedef int T14;
typedef int T15;

typedef struct Pair {
    int left;
    int right;
} Pair;

static int bench_statement_mix(int seed) {
    int acc = seed;

    {
        T0 a0 = acc;
        T1 b0 = (T1)(a0 + 1);
        acc += (a0);
        acc += (T2)b0;
        acc += ((T3)(acc));
        acc += ((Pair){ a0, b0 }).left;
    }
    {
        T4 a1 = acc;
        T5 b1 = (T5)(a1 + 2);
        acc += (a1);
        acc += (T6)b1;
        acc += ((T7)(acc));
        acc += ((Pair){ a1, b1 }).right;
    }
    {
        T8 a2 = acc;
        T9 b2 = (T9)(a2 + 3);
        acc += (a2);
        acc += (T10)b2;
        acc += ((T11)(acc));
        acc += ((Pair){ a2, b2 }).left;
    }
    {
        T12 a3 = acc;
        T13 b3 = (T13)(a3 + 4);
        acc += (a3);
        acc += (T14)b3;
        acc += ((T15)(acc));
        acc += ((Pair){ a3, b3 }).right;
    }

    acc += (T0)(acc + 5);
    acc += (T1)(acc + 6);
    acc += (T2)(acc + 7);
    acc += (T3)(acc + 8);
    acc += ((Pair){ acc, 9 }).left;
    acc += ((Pair){ acc, 10 }).right;

    return acc;
}

int main(void) {
    int total = 0;
    total += bench_statement_mix(1);
    total += bench_statement_mix(2);
    total += bench_statement_mix(3);
    return total == 0;
}
