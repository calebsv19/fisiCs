#line 5401 "virtual_types_include_assign_struct_to_int_diagjson_probe.h"
struct Payload {
    int value;
};

static int probe_wave40_include_assign_struct_to_int(void) {
    struct Payload payload = {1};
    int number = payload;
    return number;
}
