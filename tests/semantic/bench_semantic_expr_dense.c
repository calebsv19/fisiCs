int add1(int x) { return x + 1; }
int sub1(int x) { return x - 1; }
int mul2(int x) { return x * 2; }
int div2(int x) { return x / 2; }

int choose_apply(int tag, int value) {
    return tag == 0 ? add1(value)
         : tag == 1 ? sub1(value)
         : tag == 2 ? mul2(value)
                    : div2(value);
}

int fold_row(int a, int b, int c, int d) {
    int lhs = choose_apply(a & 3, b + c);
    int rhs = choose_apply(d & 3, a + c);
    int mix = (lhs > rhs ? lhs : rhs) + (lhs < rhs ? lhs : rhs);
    return mix + (((a && b) || (c && d)) ? 1 : 0);
}

int main(void) {
    int data[32] = {
        1, 3, 5, 7, 9, 11, 13, 15,
        2, 4, 6, 8, 10, 12, 14, 16,
        17, 19, 21, 23, 25, 27, 29, 31,
        18, 20, 22, 24, 26, 28, 30, 32
    };
    int total = 0;
    int i;
    for (i = 0; i < 28; ++i) {
        total += fold_row(data[i], data[i + 1], data[i + 2], data[i + 3]);
        total += choose_apply((i + total) & 3, data[i] + (total & 7));
        total += (data[i] > 10 ? data[i] : total) - (data[i] < 5 ? i : 0);
    }
    return total == 0;
}
