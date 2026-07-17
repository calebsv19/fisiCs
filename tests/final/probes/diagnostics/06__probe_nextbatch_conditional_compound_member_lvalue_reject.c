#line 3101 "virtual_lv_nextbatch_conditional_compound_member_lvalue.c"
struct ProbeConditionalLeaf {
    int slot[2];
};

int probe_nextbatch_conditional_compound_member_lvalue(int flag) {
    struct ProbeConditionalLeaf left = {{1, 2}};
    struct ProbeConditionalLeaf right = {{3, 4}};
    (flag ? left.slot[1] : right.slot[0]) += 5;
    return left.slot[1] + right.slot[0];
}
