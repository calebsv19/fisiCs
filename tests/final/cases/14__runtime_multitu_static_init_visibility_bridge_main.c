#include <stdio.h>

int wave80_step(void);
int wave80_hidden(void);

int main(void) {
    int a = wave80_step();
    int h1 = wave80_hidden();
    int b = wave80_step();
    int h2 = wave80_hidden();
    int c = wave80_step();
    int h3 = wave80_hidden();
    printf("%d %d %d %d %d %d\n", a, h1, b, h2, c, h3);
    if (a != 16 || h1 != 7 || b != 26 || h2 != 10 || c != 39 || h3 != 13) {
        return 1;
    }
    return 0;
}
