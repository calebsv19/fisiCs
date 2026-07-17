#include <limits.h>
#include <stdio.h>

int main(void) {
    int negative = -2;
    unsigned int one = 1u;
    unsigned long long high = ULLONG_MAX - 3ull;
    unsigned int selected = 1 ? (unsigned int)negative : one;
    unsigned long long promoted = 0 ? (unsigned long long)negative : high;
    int relation = negative < one;
    int equality = selected == UINT_MAX - 1u;
    int ordered = promoted > (unsigned long long)selected;

    printf("%u %llu %d %d %d\n", selected, promoted, relation, equality, ordered);
    return 0;
}
