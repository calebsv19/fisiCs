#include <stdio.h>

struct Wave21Cell {
    int scalar;
    int lane[3];
};

struct Wave21Box {
    struct Wave21Cell cells[2];
    int adjust;
};

static int wave21_selected_member_address_compound(void) {
    struct Wave21Box boxes[2] = {
        {{{5, {7, 11, 13}}, {17, {19, 23, 29}}}, 31},
        {{{37, {41, 43, 47}}, {53, {59, 61, 67}}}, 71},
    };

    int pick = boxes[1].cells[0].lane[1] > boxes[0].cells[1].lane[2];
    struct Wave21Box *selected = pick ? &boxes[1] : &boxes[0];
    struct Wave21Cell *cell = &selected->cells[pick ? 0 : 1];
    int *slot = &cell->lane[pick ? 1 : 2];

    *slot += selected->adjust;
    cell->scalar += *slot;
    selected->adjust += cell->scalar;

    return *slot + cell->scalar + selected->adjust;
}

int main(void) {
    printf("%d\n", wave21_selected_member_address_compound());
    return 0;
}
