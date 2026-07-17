#include <stdio.h>

struct Wave23QualifiedCell {
    volatile int live;
    const int frozen;
    int slots[2];
};

static int wave23_qualified_member_address(void) {
    struct Wave23QualifiedCell cells[2] = {
        {5, 7, {11, 13}},
        {17, 19, {23, 29}},
    };

    int pick = cells[1].frozen > cells[0].frozen;
    struct Wave23QualifiedCell *cell = pick ? &cells[1] : &cells[0];
    int *slot = &cell->slots[pick ? 1 : 0];

    cell->live += *slot;
    *slot += cell->frozen;

    return cell->live + *slot + cell->frozen;
}

int main(void) {
    printf("%d\n", wave23_qualified_member_address());
    return 0;
}
