#line 4501 "virtual_types_include_agg_dot_scalar_diagjson_probe.h"
struct Wrap {
    int value;
};

static int probe_wave30_include_agg_dot_scalar_base(void) {
    struct Wrap w = {3};
    return w.value.missing;
}
