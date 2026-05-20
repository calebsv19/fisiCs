#line 9311 "virtual_diagrec_include_mixed_switch_followup.h"
int f(void) {
    int x = 1;
    switch (x) {
        case 1
            x = missing;
            break;
        default:
            x = 0;
            break;
    }
    return x;
}
