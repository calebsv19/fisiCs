#line 20001 "virtual_wave87_no_prototype_real_float_array_strict.c"
typedef int (*Wave87NoPrototype)();
typedef int (*Wave87RealFloatPrototype)(float value);

extern Wave87NoPrototype wave87_unprototyped_handler;
extern Wave87RealFloatPrototype wave87_real_float_handler;

int wave87_select(void) {
    Wave87NoPrototype old_style[1] = {wave87_real_float_handler};
    Wave87RealFloatPrototype prototyped[1] = {wave87_unprototyped_handler};
    return old_style[0] != 0 && prototyped[0] != 0;
}

int main(void) {
    return wave87_select();
}
