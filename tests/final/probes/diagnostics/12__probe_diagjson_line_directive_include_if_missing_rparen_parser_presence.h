#line 8611 "virtual_diagrec_include_if_missing_rparen_case.h"
int f(void) {
    int x = 0;
    if (x > 0 {
        x = 1;
    }
    x = x + 2;
    return x;
}
