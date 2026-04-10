#line 9201 "virtual_fn_include_argument_type_mismatch_diag_text_strict.h"
int takes_ptr_inc_text(int *p) { return *p; }
int call_bad_text(void) { return takes_ptr_inc_text(1); }
