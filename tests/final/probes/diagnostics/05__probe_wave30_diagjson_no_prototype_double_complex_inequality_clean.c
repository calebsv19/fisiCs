#line 20001 "virtual_wave30_no_prototype_double_complex_inequality_clean.c"
typedef int (*NoPrototypeFunction)();
typedef int (*DoubleComplexPrototypeFunction)(double _Complex value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    DoubleComplexPrototypeFunction prototyped = 0;
    return prototyped != unprototyped ? 1 : 0;
}
