#line 7901 "virtual_scope_include_block_extern_type_conflict_probe_diagjson.h"
int shared_block_inc_probe;

static int touch_linkage_conflict_probe(void) {
    extern float shared_block_inc_probe;
    return 0;
}
