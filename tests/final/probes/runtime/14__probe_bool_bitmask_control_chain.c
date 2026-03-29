#include <stdbool.h>
#include <stdio.h>

int main(void) {
    unsigned mask = 0x5Au;
    int score = 0;

    for (unsigned i = 0; i < 12u; ++i) {
        bool b0 = (bool)(mask & 1u);
        bool b1 = (bool)(mask & 4u);
        bool b2 = (bool)(mask & 16u);

        int lane = b0 ? (b1 ? 3 : 1) : (b2 ? 2 : 0);
        switch (lane) {
            case 0: score += 5; break;
            case 1: score ^= 17; break;
            case 2: score += 9; break;
            default: score = score * 3 + 7; break;
        }

        mask = ((mask << 1) | (mask >> 7)) & 0xFFu;
    }

    printf("%u %d\n", mask, score);
    return 0;
}
