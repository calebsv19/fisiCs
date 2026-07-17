struct Wave24Flags {
    unsigned ready : 1;
    unsigned code : 7;
};

union Wave24Cell {
    struct Wave24Flags flags;
    unsigned raw;
};

int *probe_wave24_selected_union_bitfield_address(int pick) {
    union Wave24Cell cells[2] = {{{1, 3}}, {{0, 9}}};
    union Wave24Cell *selected = pick ? &cells[0] : &cells[1];
#line 12127 "virtual_lv_wave24_selected_union_bitfield_address.c"
    return (int *)&selected->flags.code;
}
