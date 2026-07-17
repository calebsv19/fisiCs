struct Wave24Locked {
    const int total;
    int open;
};

union Wave24Register {
    struct Wave24Locked locked;
    int words[2];
};

int probe_wave24_const_union_member_compound(int pick) {
    union Wave24Register regs[2] = {{{3, 4}}, {{5, 6}}};
    union Wave24Register *selected = pick ? &regs[0] : &regs[1];
#line 12153 "virtual_lv_wave24_const_union_member_compound.c"
    selected->locked.total += 1;
    return selected->locked.open;
}
