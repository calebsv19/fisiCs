#line 18801 "virtual_wave73_factory_designator_conditional_compatible_control.c"
struct SharedFactoryPayload {
    int value;
};

typedef int (*Wave73OuterCallback)(struct SharedFactoryPayload *value);
Wave73OuterCallback wave73_outer_factory(void);

int wave73_select(int choose) {
    typedef int (*Wave73InnerCallback)(struct SharedFactoryPayload *value);
    Wave73InnerCallback wave73_inner_factory(void);
    (void)(choose ? wave73_outer_factory : wave73_inner_factory);
    return 0;
}

int main(void) {
    return wave73_select(1);
}
