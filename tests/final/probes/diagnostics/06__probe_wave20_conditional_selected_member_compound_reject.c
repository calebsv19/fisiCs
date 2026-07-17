struct Wave20MemberCell {
    int values[2];
};

struct Wave20MemberBox {
    struct Wave20MemberCell cells[2];
};

int probe_wave20_conditional_selected_member_compound(int pick) {
    struct Wave20MemberBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
#line 8101 "virtual_lv_wave20_conditional_selected_member_compound.c"
    (pick ? boxes[0].cells[1].values[0] : boxes[1].cells[0].values[1]) += 3;
    return boxes[0].cells[1].values[0];
}
