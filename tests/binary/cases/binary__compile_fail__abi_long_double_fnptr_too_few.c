typedef long double (*ld_binop)(long double, long double);

static long double add_ld(long double a, long double b) {
    return a + b;
}

int main(void) {
    ld_binop fn = add_ld;
    return (int)fn(1.0L);
}
