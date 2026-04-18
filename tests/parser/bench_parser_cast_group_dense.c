typedef int I;
typedef long L;

static int bench_cast_group_dense(int seed) {
    int acc = seed;

    acc += (I)acc;
    acc += (acc);
    acc += ((I)(acc + 1));
    acc += ((acc + 2));
    acc += (L)acc;
    acc += (acc + 3);
    acc += ((L)(acc + 4));
    acc += ((acc + 5));
    acc += (I)(acc + 6);
    acc += ((acc + 7));
    acc += (L)(acc + 8);
    acc += ((acc + 9));
    acc += (I)(acc + 10);
    acc += ((acc + 11));
    acc += (L)(acc + 12);
    acc += ((acc + 13));

    return acc;
}

int main(void) {
    int total = 0;
    total += bench_cast_group_dense(1);
    total += bench_cast_group_dense(2);
    total += bench_cast_group_dense(3);
    return total == 0;
}
