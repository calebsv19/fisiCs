struct Wave23ConstSlot {
    const int locked;
    int open;
};

struct Wave23ConstBox {
    struct Wave23ConstSlot slots[2];
};

int probe_wave23_const_selected_member_postinc(int pick) {
    struct Wave23ConstBox boxes[2] = {
        {{1, 2}, {3, 4}},
        {{5, 6}, {7, 8}},
    };
    struct Wave23ConstSlot *slot = pick ? &boxes[0].slots[1] : &boxes[1].slots[0];
#line 11127 "virtual_lv_wave23_const_selected_member_postinc.c"
    return slot->locked++;
}
