#line 891 "virtual_expr_macro_def_probe_rich.c"
#define W8P_BAD_ADD() ((void)0 + 1)
#line 901 "virtual_expr_macro_use_probe_rich.c"
int probe_macro_rich(void) { return W8P_BAD_ADD(); }
