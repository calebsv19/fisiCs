#line 19101 "virtual_wave85_no_prototype_double_initializer_control.c"
typedef int (*Wave85ControlNoPrototype)();
typedef int (*Wave85ControlDoublePrototype)(double value);

extern Wave85ControlDoublePrototype wave85_double_handler;

int wave85_control_select(void) {
    Wave85ControlNoPrototype selected = wave85_double_handler;
    return selected != 0;
}

int main(void) {
    return wave85_control_select();
}
