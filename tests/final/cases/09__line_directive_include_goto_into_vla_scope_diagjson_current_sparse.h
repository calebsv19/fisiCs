#line 6602 "virtual_stmt_include_goto_vla_current.h"
static int bucket09_include_goto_vla(int n) {
    goto after_vla;
    int vla[n];
after_vla:
    return 0;
}
