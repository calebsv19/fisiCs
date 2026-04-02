#include <stdio.h>

int mt13_step(int row, int col, int x);
int mt13_state(void);

int main(void) {
    int acc = 0;
    int seed = 13;

    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            seed = (seed * 17 + row * 5 + col * 3 + 19) % 997;
            acc += mt13_step(row, col, seed) + row * 11 - col * 7;
        }
    }

    printf("%d %d\n", acc, mt13_state());
    return 0;
}
