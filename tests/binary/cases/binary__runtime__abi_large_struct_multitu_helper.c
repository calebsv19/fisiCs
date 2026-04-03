typedef struct {
    long long x[3];
    int y[4];
    double z;
} Big2;

Big2 abi_big2_make(int base) {
    Big2 value;
    value.x[0] = (long long)base * 100LL + 1LL;
    value.x[1] = (long long)base * 100LL + 2LL;
    value.x[2] = (long long)base * 100LL + 3LL;
    value.y[0] = base + 1;
    value.y[1] = base + 2;
    value.y[2] = base + 3;
    value.y[3] = base + 4;
    value.z = (double)base * 1.5;
    return value;
}

long long abi_big2_score(Big2 value) {
    long long acc = 0;
    int i;
    for (i = 0; i < 3; ++i) {
        acc += value.x[i];
    }
    for (i = 0; i < 4; ++i) {
        acc += (long long)value.y[i] * 10LL;
    }
    acc += (long long)(value.z * 20.0);
    return acc;
}
