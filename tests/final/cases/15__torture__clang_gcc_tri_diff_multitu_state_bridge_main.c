#include <stdio.h>

int mt19_step(int row, int col, int x);
int mt19_state(void);

int main(void) {
    int acc = 0;
    int seed = 19;

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 7; ++col) {
            seed = (seed * 29 + row * 11 + col * 5 + 23) % 997;
            acc += mt19_step(row, col, seed) + row * 13 - col * 9;
        }
    }

    printf("%d %d\n", acc, mt19_state());
    return 0;
}
