#include <stdio.h>

typedef struct {
    float a;
    int b;
    double c;
} TuMix;

TuMix abi_tu_make(int base);
long long abi_tu_score(TuMix value);

int main(void) {
    TuMix value = abi_tu_make(6);
    printf("%lld\n", abi_tu_score(value));
    return 0;
}
