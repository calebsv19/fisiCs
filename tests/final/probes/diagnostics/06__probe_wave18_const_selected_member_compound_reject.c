struct Wave18ConstCell {
    int value[2];
};

struct Wave18ConstBox {
    struct Wave18ConstCell cells[2];
};

int probe_wave18_const_selected_member_compound(int pick) {
    const struct Wave18ConstBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
    const struct Wave18ConstBox *selected = pick ? &boxes[0] : &boxes[1];
#line 6121 "virtual_lv_wave18_const_selected_member_compound.c"
    selected->cells[pick ? 1 : 0].value[pick ? 0 : 1] += 9;
    return selected->cells[0].value[0];
}
