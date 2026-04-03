typedef long double (*w17_op)(long double, long double);

static long double op_add(long double a, long double b) {
    return a + b;
}

static long double op_mix(long double a, long double b) {
    return a * b + 0.5L;
}

w17_op w17_pick(int id) {
    if (id == 0) return op_add;
    return op_mix;
}

long double w17_apply(w17_op fn, long double a, long double b) {
    return fn(a, b);
}
