#include <stdio.h>

extern int g_tentative_counter;

int abi_tentative_peek(void);
int abi_tentative_bump(int delta);

int main(void) {
    int a = abi_tentative_peek();
    int b = abi_tentative_bump(5);
    int c = abi_tentative_bump(-2);
    int d = g_tentative_counter;
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
