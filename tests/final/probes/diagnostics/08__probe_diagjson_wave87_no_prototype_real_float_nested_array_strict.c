#line 20101 "virtual_wave87_no_prototype_real_float_nested_array_strict.c"
typedef int (*Wave87NoPrototype)();
typedef int (*Wave87RealFloatPrototype)(float value);

struct Wave87RealFloatTable {
    Wave87NoPrototype old_style[1];
    Wave87RealFloatPrototype prototyped[1];
};

extern Wave87NoPrototype wave87_unprototyped_handler;
extern Wave87RealFloatPrototype wave87_real_float_handler;

int wave87_select(void) {
    struct Wave87RealFloatTable selected = {
        .old_style = {wave87_real_float_handler},
        .prototyped = {wave87_unprototyped_handler}
    };
    return selected.old_style[0] != 0 && selected.prototyped[0] != 0;
}

int main(void) {
    return wave87_select();
}
