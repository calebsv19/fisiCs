#line 19001 "virtual_wave85_no_prototype_float_initializer_reverse_strict.c"
typedef int (*Wave85ReverseNoPrototype)();
typedef int (*Wave85ReverseFloatPrototype)(float value);

extern Wave85ReverseNoPrototype wave85_reverse_handler;

int wave85_reverse_select(void) {
    Wave85ReverseFloatPrototype selected = wave85_reverse_handler;
    return selected != 0;
}

int main(void) {
    return wave85_reverse_select();
}
