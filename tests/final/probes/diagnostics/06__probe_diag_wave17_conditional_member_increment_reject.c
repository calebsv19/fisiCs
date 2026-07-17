struct Wave17Slot {
    int lane[3];
};

struct Wave17Carrier {
    struct Wave17Slot slots[2];
};

int probe_wave17_conditional_member_increment(int pick) {
    struct Wave17Carrier carriers[2] = {
        {{{1, 2, 3}, {4, 5, 6}}},
        {{{7, 8, 9}, {10, 11, 12}}},
    };
#line 5121 "virtual_lv_wave17_conditional_member_increment.c"
    ++(pick ? carriers[0].slots[1].lane[2] : carriers[1].slots[0].lane[1]);
    return carriers[0].slots[1].lane[2];
}
