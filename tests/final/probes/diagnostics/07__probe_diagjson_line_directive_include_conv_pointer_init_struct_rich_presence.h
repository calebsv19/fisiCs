#line 5001 "virtual_types_include_conv_pointer_init_struct_diagjson_probe.h"
struct Payload {
    int value;
};

static int probe_wave36_include_conv_pointer_init_struct(void) {
    struct Payload payload = {7};
    int *p = payload;
    return p != 0;
}
