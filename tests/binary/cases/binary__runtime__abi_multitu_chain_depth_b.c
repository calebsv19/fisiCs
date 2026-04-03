long long abi_chain_step3(long long x);

long long abi_chain_step2(long long x) {
    return abi_chain_step3(x * 5);
}
