#include <stdio.h>

int main(void) {
    int i = 0;
    int acc = 0;
    int phase = 1;

loop:
    if (i >= 14) {
        printf("%d %d\n", acc, phase);
        return 0;
    }

    switch ((i + phase) & 3) {
        case 0:
            acc += i * 3 + phase;
            ++i;
            goto loop;
        case 1:
            acc ^= i + phase * 7;
            phase = (phase + 2) & 7;
            ++i;
            goto loop;
        case 2:
            acc += i * phase - 5;
            if ((acc & 1) == 0) {
                ++phase;
                ++i;
                goto loop;
            }
            phase ^= 3;
            ++i;
            goto loop;
        default:
            acc -= i + phase;
            phase = (phase + 3) & 7;
            ++i;
            goto loop;
    }

    return 0;
}
