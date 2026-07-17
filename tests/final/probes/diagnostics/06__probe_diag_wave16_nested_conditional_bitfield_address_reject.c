#line 4121 "virtual_lv_wave16_nested_conditional_bitfield_address.c"
struct Wave16Bits {
    unsigned flags:3;
    unsigned lane:5;
};

struct Wave16BitsBox {
    struct Wave16Bits cells[2];
};

int probe_wave16_nested_conditional_bitfield_address(int pick) {
    struct Wave16BitsBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 0}}},
    };
    unsigned *ptr = &boxes[pick ? 1 : 0].cells[pick ? 0 : 1].flags;
    return ptr != 0;
}
