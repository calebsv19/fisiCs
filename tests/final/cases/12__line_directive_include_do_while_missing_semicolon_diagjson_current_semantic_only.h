#line 8811 "virtual_diagrec_include_do_while_missing_semicolon_case.h"
int f(void) {
    int x = 0;
    do {
        x = x + 1;
    } while (x < 2)
    x = x + 5;
    return x;
}
