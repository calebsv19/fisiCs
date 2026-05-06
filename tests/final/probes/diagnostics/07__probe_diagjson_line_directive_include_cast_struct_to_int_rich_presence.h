#line 3501 "virtual_types_include_cast_struct_diagjson_probe.h"
struct Pair {
    int x;
    int y;
};

static int probe_wave24_include_cast_struct_to_int(void) {
    struct Pair p = {1, 2};
    return (int)p;
}
