#line 3601 "virtual_types_agg_invalid_member_diagjson_probe.c"
struct Pair {
    int x;
    int y;
};

int main(void) {
    struct Pair p = {1, 2};
    return p.z;
}
