#include <stdio.h>

int wave45_equivalent_order(int value);
int wave45_equivalent_order();
int wave45_equivalent_order(int value);

int wave45_equivalent_order(int value) {
    return value + 4;
}

int main(void) {
    printf("%d\n", wave45_equivalent_order(41));
    return 0;
}
