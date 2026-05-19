#line 4601 "virtual_types_include_agg_union_missing_nested_field_diagjson_probe.h"
struct Pair {
    int x;
    int y;
};

union Payload {
    struct Pair pair;
    int raw[2];
};

static int probe_wave33_include_agg_union_missing_nested_field(void) {
    union Payload p;
    p.pair.x = 1;
    return p.pair.z;
}
