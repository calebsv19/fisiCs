struct Wave19Flags {
    unsigned flag : 3;
    unsigned other : 5;
};

struct Wave19FlagBox {
    struct Wave19Flags rows[2];
};

int *probe_wave19_selected_bitfield_address(int pick) {
    struct Wave19FlagBox boxes[2] = {
        {{{1, 2}, {3, 4}}},
        {{{5, 6}, {7, 8}}},
    };
    struct Wave19FlagBox *selected = pick ? &boxes[0] : &boxes[1];
#line 7141 "virtual_lv_wave19_selected_bitfield_address.c"
    return (int *)&selected->rows[pick ? 1 : 0].flag;
}
