struct Wave19IncCell {
    int value[2];
};

struct Wave19IncBox {
    struct Wave19IncCell cells[2];
};

int probe_wave19_conditional_array_member_postinc(int pick) {
    struct Wave19IncBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
#line 7101 "virtual_lv_wave19_conditional_array_member_postinc.c"
    (pick ? boxes[0].cells[1].value[0] : boxes[1].cells[0].value[1])++;
    return boxes[0].cells[1].value[0];
}
