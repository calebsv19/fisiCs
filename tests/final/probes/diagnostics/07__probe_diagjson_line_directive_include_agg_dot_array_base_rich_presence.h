#line 4901 "virtual_types_include_agg_dot_array_diagjson_probe.h"
union U {
    int raw[2];
};

static int probe_wave30_include_agg_dot_array_base(void) {
    union U u = {{1, 2}};
    return u.raw.missing;
}
