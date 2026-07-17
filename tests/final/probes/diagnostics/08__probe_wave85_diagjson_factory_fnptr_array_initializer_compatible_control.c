#line 18801 "virtual_wave85_factory_fnptr_array_initializer_compatible_control.c"
struct SharedFactoryPayload {
    int value;
};

typedef int (*Wave85OuterCallback)(struct SharedFactoryPayload *value);
typedef Wave85OuterCallback (*Wave85OuterFactory)(void);

int wave85_initialize(void) {
    typedef int (*Wave85InnerCallback)(struct SharedFactoryPayload *value);
    typedef Wave85InnerCallback (*Wave85InnerFactory)(void);
    extern Wave85InnerFactory wave85_inner_factory;
    Wave85OuterFactory selected[1] = {wave85_inner_factory};
    return selected[0] != 0;
}

int main(void) {
    return wave85_initialize();
}
