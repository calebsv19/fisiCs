#include <stdio.h>

typedef struct {
    long long x[3];
    int y[4];
    double z;
} Big2;

Big2 abi_big2_make(int base);
long long abi_big2_score(Big2 value);

int main(void) {
    Big2 value = abi_big2_make(4);
    printf("%lld\n", abi_big2_score(value));
    return 0;
}
