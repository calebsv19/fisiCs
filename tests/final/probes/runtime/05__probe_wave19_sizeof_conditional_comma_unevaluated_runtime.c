extern int printf(const char*, ...);

static int bump(int *value, int delta) {
    *value += delta;
    return *value;
}

int main(void) {
    int calls = 0;
    int guard = 0;
    int value = 9;

    int size_a = (int)sizeof((value ? (calls += 10, value) : bump(&calls, 100)));
    int chosen = value ? (guard += 3, value + 4) : (guard += 40, bump(&calls, 7));
    int size_b = (int)sizeof((guard += 90, chosen += 5));

    printf("%d %d %d %d\n", calls, guard, chosen, size_a + size_b);
    return 0;
}
