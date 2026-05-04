#include "04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict.h"

#line 16102 "virtual_decl_multitu_include_fnptr_param_conflict_probe_diag_text_lib.c"
int probe_mt_decl_fp_conflict_inc(int (*cb)(double)) {
    return cb ? 2 : 0;
}
