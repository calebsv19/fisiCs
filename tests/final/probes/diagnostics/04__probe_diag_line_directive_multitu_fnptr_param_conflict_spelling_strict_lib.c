#line 16002 "virtual_decl_multitu_fnptr_param_conflict_probe_diag_text_lib.c"
int probe_mt_decl_fp_conflict(int (*cb)(double)) {
    return cb ? 1 : 0;
}
