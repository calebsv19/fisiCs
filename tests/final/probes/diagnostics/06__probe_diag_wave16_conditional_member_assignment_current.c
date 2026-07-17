#line 4101 "virtual_lv_wave16_conditional_member_assignment.c"
struct Wave16Cell {
    int value;
};

struct Wave16Box {
    struct Wave16Cell cell;
};

int probe_wave16_conditional_member_assignment(int pick) {
    struct Wave16Box boxes[2] = {{{3}}, {{5}}};
    (pick ? boxes[0] : boxes[1]).cell.value = 7;
    return boxes[0].cell.value + boxes[1].cell.value;
}
