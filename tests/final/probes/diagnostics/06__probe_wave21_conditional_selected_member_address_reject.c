struct Wave21AddressCell {
    int lane[2];
};

struct Wave21AddressBox {
    struct Wave21AddressCell cells[2];
};

int *probe_wave21_conditional_selected_member_address(int pick) {
    struct Wave21AddressBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
#line 9101 "virtual_lv_wave21_conditional_selected_member_address.c"
    return &(pick ? boxes[0].cells[1].lane[0] : boxes[1].cells[0].lane[1]);
}
