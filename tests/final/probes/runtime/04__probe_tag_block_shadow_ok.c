struct ScopedTag {
    int a;
};

int main(void) {
    struct ScopedTag {
        int b;
    } value;
    value.b = 7;
    return value.b - 7;
}
