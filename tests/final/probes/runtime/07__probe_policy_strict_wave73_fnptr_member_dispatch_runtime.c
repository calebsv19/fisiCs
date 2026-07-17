#include <stdio.h>

typedef int (*mix_fn)(int, int);

struct Node {
    unsigned char value;
    signed char adjust;
};

struct Dispatch {
    mix_fn ops[3];
    struct Node nodes[3];
};

static int add_cast(int a, int b) {
    return (int)(unsigned char)(a + b);
}

static int sub_cast(int a, int b) {
    return (int)(signed char)(a - b);
}

static int xor_cast(int a, int b) {
    return (int)(unsigned char)(a ^ b);
}

int main(void) {
    struct Dispatch dispatch = {
        {add_cast, sub_cast, xor_cast},
        {{250u, -7}, {12u, 5}, {33u, -2}}
    };

    mix_fn *ops = dispatch.ops;
    mix_fn selected = 1 ? ops[0] : ops[2];
    mix_fn (*table)[3] = &dispatch.ops;
    struct Node *node = 0 ? &dispatch.nodes[0] : &dispatch.nodes[1];
    int first = selected(dispatch.nodes[0].value, (int)node->adjust);
    int second = (*table)[1]((int)dispatch.nodes[2].value, (int)dispatch.nodes[0].adjust);
    int third = (1 ? dispatch.ops : ops)[2]((int)node->value, (int)dispatch.nodes[2].value);

    printf("%d %d %d\n", first, second, third);
    return 0;
}
