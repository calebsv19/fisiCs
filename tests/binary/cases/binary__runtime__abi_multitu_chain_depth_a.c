long long abi_chain_step2(long long x);

long long abi_chain_step1(long long x) {
    return abi_chain_step2(x + 3);
}
