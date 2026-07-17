#line 19001 "virtual_wave75_factory_fnptr_argument_compatible_control.c"
struct SharedFactoryPayload {
    int value;
};

typedef int (*Wave75OuterCallback)(struct SharedFactoryPayload *value);
typedef Wave75OuterCallback (*Wave75OuterFactory)(void);
int wave75_consume(Wave75OuterFactory factory);

int wave75_call(void) {
    typedef int (*Wave75InnerCallback)(struct SharedFactoryPayload *value);
    typedef Wave75InnerCallback (*Wave75InnerFactory)(void);
    extern Wave75InnerFactory wave75_inner_factory;
    return wave75_consume(wave75_inner_factory);
}

int main(void) {
    return wave75_call();
}
