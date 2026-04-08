#line 4201 "virtual_init_include_nested_designator_unknown_field_probe.h"
struct Inner { int x; };
struct Outer { struct Inner inner; };

static struct Outer g = {
    .inner = {
        .missing = 1
    }
};

static int probe_include_nested_unknown_field(void) {
    return g.inner.x;
}
