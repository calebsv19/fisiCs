static int probe_exact_int(int value);

static int probe_exact_int(value)
int value;
{
    return value + 1;
}

int main(void) {
    return probe_exact_int(41) == 42 ? 0 : 1;
}
