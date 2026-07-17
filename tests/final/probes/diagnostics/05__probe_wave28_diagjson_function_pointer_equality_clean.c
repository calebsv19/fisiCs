#line 19501 "virtual_wave28_function_pointer_equality_clean.c"
typedef int (*UnaryFunction)(int value);

static int identity(int value) {
    return value;
}

int main(void) {
    UnaryFunction lhs = identity;
    UnaryFunction rhs = identity;
    return (lhs == rhs) && !(lhs != rhs) ? 0 : 1;
}
