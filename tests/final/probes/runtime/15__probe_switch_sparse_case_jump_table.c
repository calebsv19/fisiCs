#include <stdio.h>

int main(void) {
    static const int inputs[] = {
        -5000, -1024, -13, 0, 7, 19, 101, 1000,
        4096, 12345, -5000, 4096, 777, -13, 2048, 99999
    };

    int acc = 0;
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        int x = inputs[i];
        switch (x) {
            case -5000: acc += 1; break;
            case -1024: acc += 2; break;
            case -13: acc += 3; break;
            case 0: acc += 5; break;
            case 7: acc += 8; break;
            case 19: acc += 13; break;
            case 101: acc += 21; break;
            case 1000: acc += 34; break;
            case 2048: acc += 55; break;
            case 4096: acc += 89; break;
            case 12345: acc += 144; break;
            default: acc -= (x & 7); break;
        }

        switch (acc % 17) {
            case 0: acc += 10; break;
            case 3: acc += 7; break;
            case 8: acc += 4; break;
            case 16: acc -= 3; break;
            default: acc += 1; break;
        }
    }

    printf("%d\n", acc);
    return 0;
}
