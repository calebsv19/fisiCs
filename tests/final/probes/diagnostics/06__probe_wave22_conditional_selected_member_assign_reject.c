struct Wave22ConditionalCell {
    int value;
};

struct Wave22ConditionalBox {
    struct Wave22ConditionalCell cell;
};

int probe_wave22_conditional_selected_member_assign(int pick) {
    struct Wave22ConditionalBox boxes[2] = {
        {{1}},
        {{2}},
    };
#line 10101 "virtual_lv_wave22_conditional_selected_member_assign.c"
    (pick ? boxes[0] : boxes[1]).cell.value = 9;
    return boxes[0].cell.value;
}
