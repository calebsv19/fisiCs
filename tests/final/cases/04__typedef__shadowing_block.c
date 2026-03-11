typedef int scalar_t;

int typedef_shadowing_value(void) {
    typedef float scalar_t;
    scalar_t value = 0.0f;
    return (int)value;
}
