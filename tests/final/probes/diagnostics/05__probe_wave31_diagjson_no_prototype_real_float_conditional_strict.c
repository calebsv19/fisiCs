#line 20301 "virtual_wave31_no_prototype_real_float_conditional_strict.c"
typedef int (*NoPrototypeFunction)();
typedef int (*RealFloatPrototypeFunction)(float value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    RealFloatPrototypeFunction prototyped = 0;
    (void)(1 ? unprototyped : prototyped);
    return 0;
}
