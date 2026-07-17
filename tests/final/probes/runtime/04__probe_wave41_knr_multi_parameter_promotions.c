static int wave41_knr_multi(double real_value, int narrow_value);

static int wave41_knr_multi(real_value, narrow_value)
float real_value;
char narrow_value;
{
    return (int)real_value + narrow_value;
}

int main(void) {
    return wave41_knr_multi(19.0, 23) == 42 ? 0 : 1;
}
