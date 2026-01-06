int sum(int *restrict a, int *restrict b) {
    return *a + *b;
}

int main(void) {
    int x = 1;
    return sum(&x, &x);
}
