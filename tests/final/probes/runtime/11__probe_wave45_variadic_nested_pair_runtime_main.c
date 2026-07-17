#include <stdio.h>

struct Wave45Node {
    int id;
    int values[3];
};

int wave45_variadic_nested_pair(int seed, int rounds, ...);

int main(void) {
    struct Wave45Node left = { 4, { 3, 5, 7 } };
    struct Wave45Node right = { 9, { 11, 13, 17 } };
    printf("%d\n", wave45_variadic_nested_pair(23, 2, left, 6, right));
    return 0;
}
