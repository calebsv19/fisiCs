#line 19201 "virtual_wave86_no_prototype_double_array_initializer_compatible.c"
typedef int (*Wave86NoPrototype)();
typedef int (*Wave86DoublePrototype)(double value);

extern Wave86NoPrototype wave86_unprototyped_handler;
extern Wave86DoublePrototype wave86_double_handler;

int wave86_select(void) {
    Wave86NoPrototype old_style[1] = {wave86_double_handler};
    Wave86DoublePrototype prototyped[1] = {wave86_unprototyped_handler};
    return old_style[0] != 0 && prototyped[0] != 0;
}

int main(void) {
    return wave86_select();
}
