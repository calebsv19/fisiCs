#line 18501 "virtual_wave70_factory_fnptr_assignment_compatible_control.c"
struct SharedFactoryPayload {
    int value;
};

typedef int (*Wave70OuterCallback)(struct SharedFactoryPayload *value);
typedef Wave70OuterCallback (*Wave70OuterFactory)(void);
extern Wave70OuterFactory wave70_outer_factory;

int wave70_assign(void) {
    typedef int (*Wave70InnerCallback)(struct SharedFactoryPayload *value);
    typedef Wave70InnerCallback (*Wave70InnerFactory)(void);
    extern Wave70InnerFactory wave70_inner_factory;
    wave70_outer_factory = wave70_inner_factory;
    return 0;
}

int main(void) {
    return wave70_assign();
}
