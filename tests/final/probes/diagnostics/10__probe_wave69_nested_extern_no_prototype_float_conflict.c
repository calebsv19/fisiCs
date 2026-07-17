static int wave69_route_float(float value);

static int wave69_call_float(int wave69_route_float) {
    {
#line 69021 "virtual_scope_wave69_nested_extern_no_prototype_float_conflict.c"
        extern int wave69_route_float();
        return 0;
    }
}

static int wave69_route_float(float value) {
    return (int)value;
}

int main(void) {
    return wave69_call_float(7);
}
