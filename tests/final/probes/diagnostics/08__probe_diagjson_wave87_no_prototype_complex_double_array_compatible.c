#line 20201 "virtual_wave87_no_prototype_complex_double_array_compatible.c"
typedef int (*Wave87NoPrototype)();
typedef int (*Wave87ComplexDoublePrototype)(double _Complex value);

extern Wave87NoPrototype wave87_unprototyped_handler;
extern Wave87ComplexDoublePrototype wave87_complex_double_handler;

int wave87_select(void) {
    Wave87NoPrototype old_style[1] = {wave87_complex_double_handler};
    Wave87ComplexDoublePrototype prototyped[1] = {wave87_unprototyped_handler};
    return old_style[0] != 0 && prototyped[0] != 0;
}

int main(void) {
    return wave87_select();
}
