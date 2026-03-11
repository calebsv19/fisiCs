int inc(int x) {
    return x + 1;
}

int apply(int (*table[1])(int), int x) {
    return table[0](x) + 1;
}

int main(void) {
    int (*table[1])(int) = {inc};
    return apply(table, 10) - 12;
}
