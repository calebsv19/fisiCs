extern int printf(const char*, ...);

int main(void) {
    int trace = 0;
    int truthy = 1;
    int falsey = 0;

    int left = ((trace += 1), truthy) && ((trace += 2), falsey) && ((trace += 100), 1);
    int right = ((trace += 4), falsey) || ((trace += 5), truthy) || ((trace += 200), 1);
    int mixed = (((trace += 6), left) || ((trace += 7), right && ((trace += 8), 1))) && ((trace += 9), 1);

    printf("%d %d %d %d\n", trace, left, right, mixed);
    return 0;
}
