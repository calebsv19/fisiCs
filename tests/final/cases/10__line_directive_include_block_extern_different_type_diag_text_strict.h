#line 8101 "virtual_scope_include_block_extern_type_conflict_diag_text_strict.h"
int shared_block_inc_diag;

static int touch_linkage_conflict_diag(void) {
    extern float shared_block_inc_diag;
    return 0;
}
