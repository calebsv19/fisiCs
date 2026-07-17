#line 20401 "virtual_wave31_no_prototype_double_complex_conditional_clean.c"
typedef int (*NoPrototypeFunction)();
typedef int (*DoubleComplexPrototypeFunction)(double _Complex value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    DoubleComplexPrototypeFunction prototyped = 0;
    return (1 ? unprototyped : prototyped)((double _Complex)0);
}
