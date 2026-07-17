#line 19901 "virtual_wave87_no_prototype_complex_float_nested_array_compatible.c"
typedef int (*Wave87NoPrototype)();
typedef int (*Wave87ComplexFloatPrototype)(float _Complex value);

struct Wave87ComplexFloatTable {
    Wave87NoPrototype old_style[1];
    Wave87ComplexFloatPrototype prototyped[1];
};

extern Wave87NoPrototype wave87_unprototyped_handler;
extern Wave87ComplexFloatPrototype wave87_complex_float_handler;

int wave87_select(void) {
    struct Wave87ComplexFloatTable selected = {
        .old_style = {wave87_complex_float_handler},
        .prototyped = {wave87_unprototyped_handler}
    };
    return selected.old_style[0] != 0 && selected.prototyped[0] != 0;
}

int main(void) {
    return wave87_select();
}
