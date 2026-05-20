#include <stdio.h>

struct Bucket10StrongBridge {
    int base;
    int scale;
    int bias;
};

struct Bucket10StrongBridge bucket10_strong_bridge = {4, 9, 2};

int bucket10_strong_bridge_score(void);
void bucket10_strong_bridge_shift(int delta);

int main(void) {
    printf("%d ", bucket10_strong_bridge_score());
    bucket10_strong_bridge_shift(3);
    printf("%d %d\n",
           bucket10_strong_bridge_score(),
           bucket10_strong_bridge.base + bucket10_strong_bridge.scale + bucket10_strong_bridge.bias);
    return 0;
}
