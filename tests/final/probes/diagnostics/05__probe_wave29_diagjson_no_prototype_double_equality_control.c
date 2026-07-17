#line 19701 "virtual_wave29_no_prototype_double_equality_control.c"
typedef int (*NoPrototypeFunction)();
typedef int (*DoublePrototypeFunction)(double value);

int main(void) {
    NoPrototypeFunction unprototyped = 0;
    DoublePrototypeFunction prototyped = 0;
    return (unprototyped == prototyped) && !(prototyped != unprototyped) ? 0 : 1;
}
