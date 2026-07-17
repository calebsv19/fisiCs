#line 19301 "virtual_wave28_function_pointer_relational_less.c"
typedef int (*UnaryFunction)(int value);

static int increment(int value) {
    return value + 1;
}

static int decrement(int value) {
    return value - 1;
}

int main(void) {
    UnaryFunction lhs = increment;
    UnaryFunction rhs = decrement;
    return lhs < rhs;
}
