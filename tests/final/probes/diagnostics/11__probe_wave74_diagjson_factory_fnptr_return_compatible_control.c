#line 18901 "virtual_wave74_factory_fnptr_return_compatible_control.c"
struct SharedFactoryPayload {
    int value;
};

typedef int (*Wave74OuterCallback)(struct SharedFactoryPayload *value);
typedef Wave74OuterCallback (*Wave74OuterFactory)(void);

Wave74OuterFactory wave74_return(void) {
    typedef int (*Wave74InnerCallback)(struct SharedFactoryPayload *value);
    typedef Wave74InnerCallback (*Wave74InnerFactory)(void);
    extern Wave74InnerFactory wave74_inner_factory;
    return wave74_inner_factory;
}

int main(void) {
    return 0;
}
