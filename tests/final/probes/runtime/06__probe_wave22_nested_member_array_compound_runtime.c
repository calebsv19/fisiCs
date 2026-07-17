#include <stdio.h>

struct Wave22Leaf {
    int weights[2];
};

struct Wave22Row {
    struct Wave22Leaf leaves[2];
    volatile int delta;
};

struct Wave22Table {
    struct Wave22Row rows[2];
};

static int wave22_nested_member_array_compound(void) {
    struct Wave22Table tables[2] = {{{{{{0, 0}}, {{0, 0}}, 0}, {{{0, 0}}, {{0, 0}}, 0}}},
                                    {{{{{0, 0}}, {{0, 0}}, 0}, {{{0, 0}}, {{0, 0}}, 0}}}};
    const int seed = 79;

    tables[0].rows[1].leaves[0].weights[1] = 17;
    tables[1].rows[0].leaves[1].weights[0] = 43;
    tables[1].rows[0].leaves[1].weights[1] = 47;
    tables[1].rows[0].delta = 53;

    int pick = tables[1].rows[0].leaves[1].weights[0] > tables[0].rows[1].leaves[0].weights[1];
    struct Wave22Table *table = pick ? &tables[1] : &tables[0];
    struct Wave22Row *row = &table->rows[pick ? 0 : 1];
    int index = pick ? 1 : 0;

    row->leaves[index].weights[0] += row->delta;
    row->leaves[index].weights[1] += seed;
    row->delta += row->leaves[index].weights[0] - row->leaves[index].weights[1];

    return row->leaves[index].weights[0] + row->leaves[index].weights[1] + row->delta + seed;
}

int main(void) {
    printf("%d\n", wave22_nested_member_array_compound());
    return 0;
}
