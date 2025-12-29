int branch(int x) {
    return __builtin_expect(x, 0) ? 1 : 2;
}

int main(void) {
    return branch(3);
}
