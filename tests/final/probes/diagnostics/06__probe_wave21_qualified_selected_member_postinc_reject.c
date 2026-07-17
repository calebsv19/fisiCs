struct Wave21QualifiedRejectSlot {
    const int locked;
    int open;
};

struct Wave21QualifiedRejectBox {
    struct Wave21QualifiedRejectSlot slots[2];
};

int probe_wave21_qualified_selected_member_postinc(int pick) {
    struct Wave21QualifiedRejectBox boxes[2] = {
        {{1, 2}, {3, 4}},
        {{5, 6}, {7, 8}},
    };
    struct Wave21QualifiedRejectSlot *slot = pick ? &boxes[0].slots[1] : &boxes[1].slots[0];
#line 9127 "virtual_lv_wave21_qualified_selected_member_postinc.c"
    return slot->locked++;
}
