struct Wave23BitfieldSlot {
    unsigned flag : 3;
    unsigned other : 5;
};

struct Wave23BitfieldBox {
    struct Wave23BitfieldSlot slots[2];
};

int *probe_wave23_selected_bitfield_address(int pick) {
    struct Wave23BitfieldBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
    struct Wave23BitfieldBox *selected = pick ? &boxes[0] : &boxes[1];
#line 11101 "virtual_lv_wave23_selected_bitfield_address.c"
    return (int *)&selected->slots[pick ? 1 : 0].flag;
}
