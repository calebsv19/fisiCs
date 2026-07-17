#line 3121 "virtual_lv_nextbatch_nested_bitfield_address.c"
struct ProbeNestedBitfield {
    unsigned flags:3;
    unsigned lane:5;
};

struct ProbeNestedBitfieldBox {
    struct ProbeNestedBitfield cells[2];
};

int probe_nextbatch_nested_bitfield_address(int pick) {
    struct ProbeNestedBitfieldBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
    unsigned *ptr = &boxes[pick].cells[1].flags;
    return ptr != 0;
}
