#line 20201 "virtual_wave31_no_prototype_float_complex_conditional_right_arity.c"
typedef int (*NoPrototypeFunction)();
typedef int (*FloatComplexPrototypeFunction)(float _Complex value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    FloatComplexPrototypeFunction prototyped = 0;
    return (1 ? prototyped : unprototyped)();
}
