#include <stdio.h>

struct Bucket10Pair {
    int left;
    int right;
};

struct Bucket10Pair bucket10_pair;

void bucket10_pair_seed(int left, int right);
int bucket10_pair_adjust_left(int delta);
int bucket10_pair_adjust_right(int delta);
int bucket10_pair_total(void);

int main(void) {
    bucket10_pair_seed(8, 13);
    printf("%d %d %d\n",
           bucket10_pair_adjust_left(5),
           bucket10_pair_adjust_right(-3),
           bucket10_pair_total());
    return 0;
}
