#line 19401 "virtual_wave28_function_pointer_relational_greater_equal.c"
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
    return lhs >= rhs;
}
