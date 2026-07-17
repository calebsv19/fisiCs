#line 19001 "virtual_wave86_no_prototype_float_array_initializer_strict.c"
typedef int (*Wave86NoPrototype)();
typedef int (*Wave86FloatPrototype)(float value);

extern Wave86FloatPrototype wave86_float_handler;

int wave86_select(void) {
    Wave86NoPrototype selected[1] = {wave86_float_handler};
    return selected[0] != 0;
}

int main(void) {
    return wave86_select();
}
