#line 19701 "virtual_wave86_no_prototype_complex_double_compatible.c"
typedef int (*Wave86NoPrototype)();
typedef int (*Wave86ComplexDoublePrototype)(double _Complex value);

extern Wave86NoPrototype wave86_unprototyped_handler;
extern Wave86ComplexDoublePrototype wave86_complex_double_handler;

int wave86_bridge(void) {
    Wave86NoPrototype initialized_old = wave86_complex_double_handler;
    Wave86ComplexDoublePrototype initialized_prototyped = wave86_unprototyped_handler;
    Wave86NoPrototype assigned_old = 0;
    Wave86ComplexDoublePrototype assigned_prototyped = 0;
    assigned_old = wave86_complex_double_handler;
    assigned_prototyped = wave86_unprototyped_handler;
    return initialized_old != 0 && initialized_prototyped != 0
        && assigned_old != 0 && assigned_prototyped != 0;
}

int main(void) {
    return wave86_bridge();
}
