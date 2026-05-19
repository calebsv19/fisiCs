#line 3901 "virtual_types_include_agg_union_arrow_nonptr_diagjson_probe.h"
struct Pair {
    int x;
    int y;
};

union Payload {
    struct Pair pair;
    int raw[2];
};

static int probe_wave26_include_agg_union_arrow_nonptr(void) {
    union Payload p;
    p.pair.x = 1;
    return p->pair.x;
}
