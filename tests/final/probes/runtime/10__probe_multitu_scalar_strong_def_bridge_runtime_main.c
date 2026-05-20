#include <stdio.h>

int bucket10_scalar_bridge = 9;

int bucket10_scalar_bridge_score(void);
void bucket10_scalar_bridge_shift(int delta);

int main(void) {
    int before;
    int after;
    int raw;

    before = bucket10_scalar_bridge_score();
    bucket10_scalar_bridge_shift(4);
    after = bucket10_scalar_bridge_score();
    raw = bucket10_scalar_bridge;
    printf("%d %d %d\n", before, after, raw);
    return 0;
}
