#line 4401 "virtual_types_include_cast_int_to_struct_diagjson_probe.h"
struct Pair {
    int x;
    int y;
};

static int probe_wave30_include_cast_int_to_struct(void) {
    int v = 5;
    struct Pair p = (struct Pair)v;
    return p.x;
}
