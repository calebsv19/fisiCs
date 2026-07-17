static int probe_promoted_char(int value);

static int probe_promoted_char(value)
char value;
{
    return value + 1;
}

int main(void) {
    return probe_promoted_char(41) == 42 ? 0 : 1;
}
