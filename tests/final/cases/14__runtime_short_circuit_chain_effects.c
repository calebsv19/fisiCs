#include <stdio.h>

static int trace = 0;

static int mark(int tag, int value) {
    trace = (trace * 10) + tag;
    return value;
}

int main(void) {
    int a = 0;
    int b = 0;

    if (mark(1, 1) && mark(2, 0) && mark(3, 1)) {
        a = 1;
    }

    if (mark(4, 0) || mark(5, 1)) {
        b = 1;
    }

    printf("%d %d %d\n", trace, a, b);
    return 0;
}
