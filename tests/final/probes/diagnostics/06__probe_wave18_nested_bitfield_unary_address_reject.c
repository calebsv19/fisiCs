struct Wave18Bits {
    unsigned flag:3;
    unsigned code:5;
};

struct Wave18BitsBox {
    struct Wave18Bits bits[2];
};

int probe_wave18_nested_bitfield_unary_address(int pick) {
    struct Wave18BitsBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 0}}},
    };
#line 6141 "virtual_lv_wave18_nested_bitfield_unary_address.c"
    unsigned *ptr = &(*(pick ? &boxes[0] : &boxes[1])).bits[pick ? 1 : 0].flag;
    return ptr != 0;
}
