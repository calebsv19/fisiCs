int good(int a, const char* fmt, ...) {
    __builtin_va_list ap;
    va_start(ap, fmt); // OK: variadic, has fixed params
    return a;
}

int bad1(...) {
    __builtin_va_list ap;
    va_start(ap, fmt); // no fixed params, also undefined fmt
    return 0;
}

int bad2(int a) {
    __builtin_va_list ap;
    va_start(ap, a); // non-variadic function
    return a;
}

int main(void) {
    good(1, "x", 2);
    bad1();
    bad2(3);
    return 0;
}
