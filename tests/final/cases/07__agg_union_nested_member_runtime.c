#include <stdio.h>

struct Pair {
    int x;
    int y;
};

union Payload {
    struct Pair pair;
    int raw[2];
};

struct Outer {
    int tag;
    union Payload payload;
    struct Pair extra;
};

int main(void) {
    struct Outer o;
    o.tag = 3;
    o.payload.pair.x = 10;
    o.payload.pair.y = 20;
    o.extra.x = 5;
    o.extra.y = 7;

    int total = o.tag + o.payload.pair.x + o.payload.pair.y + o.extra.x + o.extra.y;
    printf("%d\n", total);
    return 0;
}
