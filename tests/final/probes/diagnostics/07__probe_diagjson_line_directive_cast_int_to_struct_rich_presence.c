#line 4201 "virtual_types_cast_int_to_struct_diagjson_probe.c"
struct Pair {
    int x;
    int y;
};

int main(void) {
    int v = 5;
    struct Pair p = (struct Pair)v;
    return p.x;
}
