extern int printf(const char*, ...);

int main(void) {
    int left = 2;
    int right = 4;
    int value = 0 ? (left += 10) : (right += 6);

    printf("%d %d %d\n", left, right, value);
    return 0;
}
