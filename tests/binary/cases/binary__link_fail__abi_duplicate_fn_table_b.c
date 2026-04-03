static int plus_two(int x) {
    return x + 2;
}

int (*abi_table[1])(int) = {plus_two};
