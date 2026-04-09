#line 7701 "virtual_scope_include_block_extern_type_conflict_diagjson_strict.h"
int shared_block_inc;

static int touch_linkage_conflict(void) {
    extern float shared_block_inc;
    return 0;
}
