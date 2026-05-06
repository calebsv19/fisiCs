static int bump_pair(int *restrict left, int *restrict right) {
    *left += 2;
    *right -= 1;
    return *left + *right;
}

int main(void) {
    int a = 3;
    int b = 9;
    return bump_pair(&a, &b);
}
