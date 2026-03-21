#include <stdio.h>

typedef int (*op_t)(int);

static int add_one(int x) {
    return x + 1;
}

static int mul_two(int x) {
    return x * 2;
}

static int sub_three(int x) {
    return x - 3;
}

int main(void) {
    op_t ops[3] = {add_one, mul_two, sub_three};
    int total = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        int idx = i % 3;
        int v = 0;

        switch (idx) {
            case 0:
                v = ops[idx](i);
                break;
            case 1:
                v = ops[idx](i + 2);
                break;
            default:
                v = ops[idx](i + 5);
                break;
        }

        total += v;
    }

    printf("%d\n", total);
    return 0;
}
