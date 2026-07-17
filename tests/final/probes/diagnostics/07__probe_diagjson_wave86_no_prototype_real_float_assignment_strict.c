#line 19601 "virtual_wave86_no_prototype_real_float_assignment_strict.c"
typedef int (*Wave86NoPrototype)();
typedef int (*Wave86RealFloatPrototype)(float value);

extern Wave86NoPrototype wave86_unprototyped_handler;
extern Wave86RealFloatPrototype wave86_real_float_handler;

int wave86_assign(void) {
    Wave86NoPrototype old_style = 0;
    Wave86RealFloatPrototype prototyped = 0;
    old_style = wave86_real_float_handler;
    prototyped = wave86_unprototyped_handler;
    return old_style != 0 && prototyped != 0;
}

int main(void) {
    return wave86_assign();
}
