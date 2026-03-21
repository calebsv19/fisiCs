int inc(int x) {
    return x + 1;
}

int dec(int x) {
    return x - 1;
}

int main(void) {
    int (*ops[2])(int) = {inc, dec};
    int a = ops[0](7);
    int b = ops[1](7);
    return a * 10 + b;
}
