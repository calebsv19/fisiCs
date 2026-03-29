#include <stddef.h>
#include <stdio.h>

int *load_store(int seed);
int checksum_store(int step);

int main(void) {
    int *first = load_store(9);
    int seed_a = first[2] + first[9];

    int *second = load_store(3);
    ptrdiff_t delta = second - first;

    int score = checksum_store(2) + checksum_store(3);
    printf("%d %ld %d\n", seed_a, (long)delta, score);
    return 0;
}
