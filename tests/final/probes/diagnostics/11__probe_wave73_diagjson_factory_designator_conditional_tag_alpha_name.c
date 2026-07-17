#line 18801 "virtual_wave73_factory_designator_conditional_tag_alpha_name.c"
struct FactoryPayload {
    int outer_value;
};

typedef int (*Wave73OuterCallback)(struct FactoryPayload *value);
Wave73OuterCallback wave73_outer_factory(void);

int wave73_select(int choose) {
    struct InnerFactoryPayload {
        long inner_value;
        long extra;
    };
    typedef int (*Wave73InnerCallback)(struct InnerFactoryPayload *value);
    Wave73InnerCallback wave73_inner_factory(void);
    (void)(choose ? wave73_outer_factory : wave73_inner_factory);
    return 0;
}

int main(void) {
    return wave73_select(1);
}
