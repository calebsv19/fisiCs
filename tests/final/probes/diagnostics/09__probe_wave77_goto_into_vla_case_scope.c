#line 7701 "virtual_stmt_wave77_goto_into_vla_case_scope.c"
int main(void) {
    int selector = 1;

    goto inside_wave77_vla_case;

    switch (selector) {
        default:
            return 0;

        case 1: {
            int n = selector + 3;
            int row[n];
inside_wave77_vla_case:
            row[0] = 11;
            return row[0];
        }
    }
}
