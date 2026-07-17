struct wave43_paren_pair {
    int left;
    int right;
};

int main(void) {
#line 14321 "virtual_wave43_compound_unclosed_paren.c"
    struct wave43_paren_pair pair = (struct wave43_paren_pair){.left (1, 2, .right = 3};
    return wave43_paren_tail_missing;
}
