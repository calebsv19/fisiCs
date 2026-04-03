int abi_dispatch_pair(int x) {
    return x + 1;
}

int main(void) {
    return abi_dispatch_pair(3);
}
