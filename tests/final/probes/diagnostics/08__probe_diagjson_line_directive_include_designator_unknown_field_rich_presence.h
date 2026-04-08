#line 3401 "virtual_init_include_designator_unknown_field_probe.h"
struct Record {
    int id;
};

static int probe_include_designator_unknown_field(void) {
    struct Record value = {.missing = 1};
    return value.id;
}
