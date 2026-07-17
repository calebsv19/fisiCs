struct Wave18Slot {
    int value[2];
};

struct Wave18Box {
    struct Wave18Slot slots[2];
};

int probe_wave18_conditional_deref_compound(int pick) {
    struct Wave18Box boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
    int *left = &boxes[0].slots[1].value[0];
    int *right = &boxes[1].slots[0].value[1];
#line 6101 "virtual_lv_wave18_conditional_deref_compound.c"
    (pick ? *left : *right) += boxes[pick ? 0 : 1].slots[pick ? 1 : 0].value[1];
    return boxes[0].slots[1].value[0];
}
