#line 3701 "virtual_types_include_agg_invalid_member_diagjson_probe.h"
struct Pair {
    int x;
    int y;
};

static int probe_wave24_include_agg_invalid_member(void) {
    struct Pair p = {1, 2};
    return p.z;
}
