#line 3851 "virtual_init_include_nested_designator_unknown_field_header_case.h"
struct Inner { int x; };
struct Outer { struct Inner inner; };

static struct Outer g = {
    .inner = {
        .missing = 1
    }
};

static int wave4_include_nested_unknown_field(void) {
    return g.inner.x;
}
