struct Wave20Bits {
    unsigned lo : 4;
    unsigned hi : 4;
};

struct Wave20BitRow {
    struct Wave20Bits bits[2];
};

struct Wave20BitBox {
    struct Wave20BitRow rows[2];
};

int *probe_wave20_nested_selected_bitfield_address(int pick) {
    struct Wave20BitBox boxes[2] = {
        {{{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}}},
        {{{{9, 10}, {11, 12}}, {{13, 14}, {15, 0}}}},
    };
    struct Wave20BitRow *row = pick ? &boxes[0].rows[1] : &boxes[1].rows[0];
#line 8125 "virtual_lv_wave20_nested_selected_bitfield_address.c"
    return (int *)&row->bits[pick ? 0 : 1].hi;
}
