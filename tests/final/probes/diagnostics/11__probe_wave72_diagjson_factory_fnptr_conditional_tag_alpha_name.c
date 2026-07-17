#line 18701 "virtual_wave72_factory_fnptr_conditional_tag_alpha_name.c"
struct FactoryPayload {
    int outer_value;
};

typedef int (*Wave72OuterCallback)(struct FactoryPayload *value);
typedef Wave72OuterCallback (*Wave72OuterFactory)(void);
extern Wave72OuterFactory wave72_outer_factory;

int wave72_select(int choose) {
    struct InnerFactoryPayload {
        long inner_value;
        long extra;
    };
    typedef int (*Wave72InnerCallback)(struct InnerFactoryPayload *value);
    typedef Wave72InnerCallback (*Wave72InnerFactory)(void);
    extern Wave72InnerFactory wave72_inner_factory;
    (void)(choose ? wave72_outer_factory : wave72_inner_factory);
    return 0;
}

int main(void) {
    return wave72_select(1);
}
