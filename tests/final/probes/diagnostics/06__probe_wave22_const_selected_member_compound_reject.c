struct Wave22ConstSlot {
    const int locked;
    int open;
};

struct Wave22ConstBox {
    struct Wave22ConstSlot slots[2];
};

int probe_wave22_const_selected_member_compound(int pick) {
    struct Wave22ConstBox boxes[2] = {
        {{1, 2}, {3, 4}},
        {{5, 6}, {7, 8}},
    };
    struct Wave22ConstSlot *slot = pick ? &boxes[0].slots[1] : &boxes[1].slots[0];
#line 10127 "virtual_lv_wave22_const_selected_member_compound.c"
    slot->locked += 1;
    return slot->open;
}
