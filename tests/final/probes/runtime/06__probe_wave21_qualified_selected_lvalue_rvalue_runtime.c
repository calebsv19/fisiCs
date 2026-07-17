#include <stdio.h>

struct Wave21QualifiedSlot {
    volatile int counter;
    const int scale;
    int value;
};

struct Wave21QualifiedBox {
    struct Wave21QualifiedSlot slots[2];
};

static int wave21_qualified_selected_lvalue_rvalue(void) {
    struct Wave21QualifiedBox boxes[2] = {
        {{{3, 5, 7}, {11, 13, 17}}},
        {{{19, 23, 29}, {31, 37, 41}}},
    };

    int pick = boxes[0].slots[1].scale < boxes[1].slots[0].scale;
    struct Wave21QualifiedSlot *selected = pick ? &boxes[1].slots[0] : &boxes[0].slots[1];
    volatile int *counter = &selected->counter;
    const int *scale = &selected->scale;

    int before = (*counter)++;
    selected->value += before + *scale;
    *counter += selected->value;

    return before + *counter + selected->value + *scale;
}

int main(void) {
    printf("%d\n", wave21_qualified_selected_lvalue_rvalue());
    return 0;
}
