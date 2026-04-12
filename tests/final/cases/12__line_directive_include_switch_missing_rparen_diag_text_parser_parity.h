#line 8911 "virtual_diagrec_include_switch_missing_rparen_case.h"
int f(void) {
    int x = 0;
    switch (x {
        case 0:
            x = 1;
            break;
        default:
            x = 2;
            break;
    }
    return x;
}
