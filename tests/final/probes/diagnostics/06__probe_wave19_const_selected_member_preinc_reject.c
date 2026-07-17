struct Wave19ConstCell {
    int value[2];
};

struct Wave19ConstBox {
    struct Wave19ConstCell cells[2];
};

int probe_wave19_const_selected_member_preinc(int pick) {
    const struct Wave19ConstBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
    const struct Wave19ConstBox *selected = pick ? &boxes[1] : &boxes[0];
#line 7121 "virtual_lv_wave19_const_selected_member_preinc.c"
    ++selected->cells[pick ? 0 : 1].value[pick ? 1 : 0];
    return selected->cells[0].value[0];
}
