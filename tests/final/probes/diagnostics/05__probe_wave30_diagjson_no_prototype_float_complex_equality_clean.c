#line 19801 "virtual_wave30_no_prototype_float_complex_equality_clean.c"
typedef int (*NoPrototypeFunction)();
typedef int (*FloatComplexPrototypeFunction)(float _Complex value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    FloatComplexPrototypeFunction prototyped = 0;
    return (unprototyped == prototyped) && !(prototyped != unprototyped) ? 0 : 1;
}
