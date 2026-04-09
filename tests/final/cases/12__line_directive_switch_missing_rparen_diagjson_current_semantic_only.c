#line 8901 "virtual_diagrec_switch_missing_rparen_case.c"
int main(void) {
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
