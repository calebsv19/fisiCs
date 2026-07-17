#line 19901 "virtual_wave30_no_prototype_real_float_equality_strict.c"
typedef int (*NoPrototypeFunction)();
typedef int (*RealFloatPrototypeFunction)(float value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    RealFloatPrototypeFunction prototyped = 0;
    return unprototyped == prototyped;
}
