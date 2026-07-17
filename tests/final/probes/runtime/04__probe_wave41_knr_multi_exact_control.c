static int wave41_knr_multi_exact(double real_value, int integer_value);

static int wave41_knr_multi_exact(real_value, integer_value)
double real_value;
int integer_value;
{
    return (int)real_value + integer_value;
}

int main(void) {
    return wave41_knr_multi_exact(19.0, 23) == 42 ? 0 : 1;
}
