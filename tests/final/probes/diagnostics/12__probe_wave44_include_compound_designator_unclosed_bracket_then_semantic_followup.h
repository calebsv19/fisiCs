struct wave44_include_pair {
    int left;
    int right;
};

#line 14441 "virtual_wave44_compound_designator_include.h"
static int wave44_include_bad(int *values) {
    struct wave44_include_pair pair = (struct wave44_include_pair){.left values[0, .right = 3};
    return pair.right;
}
