typedef struct {
    int x;
    long long y;
} AbiPair;

AbiPair abi_make_pair(int base) {
    AbiPair pair;
    pair.x = base + 1;
    pair.y = (long long)base * 100LL + 11LL;
    return pair;
}

long long abi_pair_score(AbiPair pair, int k) {
    return (long long)pair.x * (long long)k + pair.y;
}
