#include <stdio.h>

typedef struct Toggle {
    _Bool visible;
} Toggle;

static int pick_visible(Toggle t, int gate) {
    return gate ? t.visible : 0;
}

int main(void) {
    Toggle on = {1};
    Toggle off = {0};
    int a = pick_visible(on, 1);
    int b = pick_visible(on, 0);
    int c = pick_visible(off, 1);
    int d = (+1) + a + b + c;
    printf("a=%d b=%d c=%d d=%d\n", a, b, c, d);
    return 0;
}
