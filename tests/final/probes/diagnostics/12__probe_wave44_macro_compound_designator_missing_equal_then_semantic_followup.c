struct wave44_macro_pair {
    int left;
    int right;
};

#define W44_PAIR(left_value) \
    ((struct wave44_macro_pair){.left left_value, .right = 2})

int main(void) {
#line 14401 "virtual_wave44_macro_compound_callsite.c"
    struct wave44_macro_pair pair = W44_PAIR(1);
    return wave44_macro_tail_missing;
}
