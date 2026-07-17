#line 19301 "virtual_wave86_no_prototype_complex_float_compatible.c"
typedef int (*Wave86NoPrototype)();
typedef int (*Wave86ComplexFloatPrototype)(float _Complex value);

extern Wave86NoPrototype wave86_unprototyped_handler;
extern Wave86ComplexFloatPrototype wave86_complex_float_handler;

int wave86_select(void) {
    Wave86NoPrototype old_style = wave86_complex_float_handler;
    Wave86ComplexFloatPrototype prototyped = wave86_unprototyped_handler;
    return old_style != 0 && prototyped != 0;
}

int main(void) {
    return wave86_select();
}
