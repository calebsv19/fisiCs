#line 5901 "virtual_types_nested_designator_unknown_field_probe.c"
struct Inner { int x; };
struct Outer { struct Inner inner; };

struct Outer g = {
    .inner = {
        .missing = 1
    }
};

int main(void) {
    return g.inner.x;
}
