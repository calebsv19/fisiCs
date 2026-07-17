#line 19601 "virtual_wave29_no_prototype_float_equality_strict.c"
typedef int (*NoPrototypeFunction)();
typedef int (*FloatPrototypeFunction)(float value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    FloatPrototypeFunction prototyped = 0;
    return unprototyped == prototyped;
}
