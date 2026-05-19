#line 4301 "virtual_types_agg_dot_scalar_diagjson_probe.c"
struct Wrap {
    int value;
};

int main(void) {
    struct Wrap w = {3};
    return w.value.missing;
}
