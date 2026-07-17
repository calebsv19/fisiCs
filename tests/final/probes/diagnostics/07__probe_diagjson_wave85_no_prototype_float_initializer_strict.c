#line 18901 "virtual_wave85_no_prototype_float_initializer_strict.c"
typedef int (*Wave85NoPrototype)();
typedef int (*Wave85FloatPrototype)(float value);

extern Wave85FloatPrototype wave85_float_handler;

int wave85_select(void) {
    Wave85NoPrototype selected = wave85_float_handler;
    return selected != 0;
}

int main(void) {
    return wave85_select();
}
