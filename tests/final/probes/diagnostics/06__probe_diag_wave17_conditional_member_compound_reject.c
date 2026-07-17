struct Wave17Leaf {
    int value[2];
};

struct Wave17Box {
    struct Wave17Leaf leaf[2];
};

int probe_wave17_conditional_member_compound(int pick) {
    struct Wave17Box boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
#line 5101 "virtual_lv_wave17_conditional_member_compound.c"
    (pick ? boxes[0].leaf[1].value[0] : boxes[1].leaf[0].value[1]) += 9;
    return boxes[0].leaf[1].value[0];
}
