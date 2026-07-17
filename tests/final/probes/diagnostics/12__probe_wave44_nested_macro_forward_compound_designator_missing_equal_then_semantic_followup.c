struct wave44_nested_pair {
    int left;
    int right;
};

#define W44_NESTED_BUILD(value) \
    ((struct wave44_nested_pair){.left value, .right = 4})
#define W44_NESTED_FORWARD(value) W44_NESTED_BUILD(value)

int main(void) {
#line 14481 "virtual_wave44_nested_macro_forward_callsite.c"
    struct wave44_nested_pair pair = W44_NESTED_FORWARD(9);
    return wave44_nested_macro_tail_missing;
}
