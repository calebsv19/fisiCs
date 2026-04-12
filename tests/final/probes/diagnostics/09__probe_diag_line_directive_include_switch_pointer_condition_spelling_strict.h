#line 7812 "virtual_stmt_include_switch_pointer_diag_probe.h"
int main(void) {
    int x = 0;
    int *p = &x;
    switch (p) {
        case 0:
            return 0;
    }
    return 1;
}
