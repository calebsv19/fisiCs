#include <stdio.h>

typedef struct {
    int cells[2][3];
    int adjust;
} Matrix;

int main(void) {
    Matrix mats[2] = {
        {{{1, 2, 3}, {4, 5, 6}}, 7},
        {{{8, 9, 10}, {11, 12, 13}}, 14},
    };

    int pick = mats[1].cells[0][1] > mats[0].adjust;
    int *first = &mats[pick].cells[1][0];
    *first += mats[pick].adjust;

    mats[pick ? 0 : 1].cells[pick][2] += *first;

    int (*row)[3] = pick ? &mats[0].cells[1] : &mats[1].cells[0];
    (*row)[pick + 1] += mats[1].cells[1][0];

    int total = *first + mats[0].cells[1][2] + mats[1].cells[0][1] + mats[0].adjust;
    printf("%d %d %d %d\n", *first, mats[0].cells[1][2], mats[1].cells[0][1], total);
    return 0;
}
