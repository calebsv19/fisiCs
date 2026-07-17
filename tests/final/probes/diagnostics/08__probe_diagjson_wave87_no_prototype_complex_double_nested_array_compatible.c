#line 20301 "virtual_wave87_no_prototype_complex_double_nested_array_compatible.c"
typedef int (*Wave87NoPrototype)();
typedef int (*Wave87ComplexDoublePrototype)(double _Complex value);

struct Wave87ComplexDoubleTable {
    Wave87NoPrototype old_style[1];
    Wave87ComplexDoublePrototype prototyped[1];
};

extern Wave87NoPrototype wave87_unprototyped_handler;
extern Wave87ComplexDoublePrototype wave87_complex_double_handler;

int wave87_select(void) {
    struct Wave87ComplexDoubleTable selected = {
        .old_style = {wave87_complex_double_handler},
        .prototyped = {wave87_unprototyped_handler}
    };
    return selected.old_style[0] != 0 && selected.prototyped[0] != 0;
}

int main(void) {
    return wave87_select();
}
