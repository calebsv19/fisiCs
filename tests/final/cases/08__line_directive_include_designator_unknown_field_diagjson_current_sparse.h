#line 3201 "virtual_init_include_designator_unknown_field_header_case.h"
struct Record {
    int id;
};

static int wave3_include_designator_unknown_field(void) {
    struct Record value = {.missing = 1};
    return value.id;
}
