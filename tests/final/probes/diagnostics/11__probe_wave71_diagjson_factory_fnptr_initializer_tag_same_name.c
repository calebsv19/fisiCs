#line 18601 "virtual_wave71_factory_fnptr_initializer_tag_same_name.c"
struct FactoryPayload {
    int outer_value;
};

typedef int (*Wave71OuterCallback)(struct FactoryPayload *value);
typedef Wave71OuterCallback (*Wave71OuterFactory)(void);

int wave71_initialize(void) {
    struct FactoryPayload {
        long inner_value;
        long extra;
    };
    typedef int (*Wave71InnerCallback)(struct FactoryPayload *value);
    typedef Wave71InnerCallback (*Wave71InnerFactory)(void);
    extern Wave71InnerFactory wave71_inner_factory;
    Wave71OuterFactory selected = wave71_inner_factory;
    return selected != 0;
}

int main(void) {
    return wave71_initialize();
}
