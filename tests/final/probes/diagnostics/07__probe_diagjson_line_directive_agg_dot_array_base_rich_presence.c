#line 4801 "virtual_types_agg_dot_array_diagjson_probe.c"
union U {
    int raw[2];
};

int main(void) {
    union U u = {{1, 2}};
    return u.raw.missing;
}
