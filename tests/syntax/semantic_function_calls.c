int takes_two(int count, const char* label);
int takes_two(int count, const char* label) {
    return count;
}

int takes_mutable(int* value);
int takes_mutable(int* value) {
    return *value;
}

int variadic_example(int first, ...);
int variadic_example(int first, ...) {
    return first;
}

void exercise(void) {
    int local = takes_two(1, "ok");

    takes_two(1);                 // too few
    takes_two(1, "extra", 3);     // too many
    takes_two("oops", "ok");      // wrong type

    const int c = 42;
    takes_mutable(&c);            // qualifier drop

    variadic_example(1);
    variadic_example(1, 2.0f);    // float promotes to double

    (void)local;
}
