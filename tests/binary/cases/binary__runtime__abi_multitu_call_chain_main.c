#include <stdio.h>

typedef struct {
    int x;
    long long y;
} AbiPair;

AbiPair abi_make_pair(int base);
long long abi_pair_score(AbiPair pair, int k);

int main(void) {
    AbiPair pair = abi_make_pair(7);
    printf("%lld\n", abi_pair_score(pair, 3));
    return 0;
}
