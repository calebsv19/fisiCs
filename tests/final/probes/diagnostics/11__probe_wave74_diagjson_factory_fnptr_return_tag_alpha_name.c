#line 18901 "virtual_wave74_factory_fnptr_return_tag_alpha_name.c"
struct FactoryPayload {
    int outer_value;
};

typedef int (*Wave74OuterCallback)(struct FactoryPayload *value);
typedef Wave74OuterCallback (*Wave74OuterFactory)(void);

Wave74OuterFactory wave74_return(void) {
    struct InnerFactoryPayload {
        long inner_value;
        long extra;
    };
    typedef int (*Wave74InnerCallback)(struct InnerFactoryPayload *value);
    typedef Wave74InnerCallback (*Wave74InnerFactory)(void);
    extern Wave74InnerFactory wave74_inner_factory;
    return wave74_inner_factory;
}

int main(void) {
    return 0;
}
