#include <stdio.h>

typedef struct LargePayload {
    unsigned char bytes[131072];
    int marker;
} LargePayload;

LargePayload make_large_payload(int seed);

static int exercise(int select_left) {
    LargePayload left = {0};
    left.bytes[0] = 3;
    left.bytes[65535] = 7;
    left.bytes[131071] = 11;
    left.marker = 19;

    LargePayload selected = select_left ? left : (LargePayload){0};
    return selected.bytes[0] + selected.bytes[65535] +
           selected.bytes[131071] + selected.marker;
}

int main(void) {
    LargePayload from_call = make_large_payload(5);
    int call_sum = from_call.bytes[0] + from_call.bytes[131071] + from_call.marker;
    printf("large-ternary=%d,%d call=%d\n", exercise(1), exercise(0), call_sum);
    return 0;
}
