#line 19101 "virtual_wave86_no_prototype_float_array_initializer_reverse_strict.c"
typedef int (*Wave86NoPrototype)();
typedef int (*Wave86FloatPrototype)(float value);

extern Wave86NoPrototype wave86_unprototyped_handler;

int wave86_select(void) {
    Wave86FloatPrototype selected[1] = {wave86_unprototyped_handler};
    return selected[0] != 0;
}

int main(void) {
    return wave86_select();
}
