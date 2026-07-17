#line 7621 "virtual_stmt_switch_goto_vla_case_wave76.c"
int main(void) {
    int selector = 0;

    goto inside_vla_case;

    switch (selector) {
        case 0: {
            int n = selector + 2;
            int row[n];
inside_vla_case:
            row[0] = 9;
            return row[0];
        }

        default:
            return 0;
    }
}
