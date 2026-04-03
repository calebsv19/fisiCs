static int plus_one(int x) {
    return x + 1;
}

int (*abi_table[1])(int) = {plus_one};

int main(void) {
    return abi_table[0](2);
}
