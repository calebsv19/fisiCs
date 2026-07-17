#line 20101 "virtual_wave31_no_prototype_float_complex_conditional_left_arity.c"
typedef int (*NoPrototypeFunction)();
typedef int (*FloatComplexPrototypeFunction)(float _Complex value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    FloatComplexPrototypeFunction prototyped = 0;
    return (1 ? unprototyped : prototyped)();
}
