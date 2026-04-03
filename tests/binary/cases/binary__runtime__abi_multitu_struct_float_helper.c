typedef struct {
    float a;
    int b;
    double c;
} TuMix;

TuMix abi_tu_make(int base) {
    TuMix value;
    value.a = (float)base * 0.5f;
    value.b = base + 2;
    value.c = (double)base * 1.25;
    return value;
}

long long abi_tu_score(TuMix value) {
    return (long long)(value.a * 8.0f) +
           (long long)value.b * 10LL +
           (long long)(value.c * 8.0);
}
