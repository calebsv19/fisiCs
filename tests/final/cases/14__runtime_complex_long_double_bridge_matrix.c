#include <complex.h>
#include <stdio.h>

struct Wave83ComplexNode {
    long double complex z;
    long double bias;
};

static long long quant8(long double x) {
    if (x >= 0.0L) {
        return (long long)(x * 8.0L + 0.5L);
    }
    return (long long)(x * 8.0L - 0.5L);
}

static long double complex apply_node(struct Wave83ComplexNode n, long double complex k) {
    return n.z * k + n.bias;
}

int main(void) {
    long double complex a = 1.5L + 2.0Li;
    long double complex b = -0.5L + 1.0Li;
    long double complex c = a * b;
    long double complex d = a + b;

    struct Wave83ComplexNode node;
    node.z = 0.25L - 1.5Li;
    node.bias = 0.75L;

    long double complex e = apply_node(node, d);
    long double r = creall(c) + creall(e);
    long double i = cimagl(c) + cimagl(e);
    printf("%lld %lld\n", quant8(r), quant8(i));
    return 0;
}
