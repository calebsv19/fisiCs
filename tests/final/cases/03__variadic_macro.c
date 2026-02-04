#define LOG(fmt, ...) fmt, __VA_ARGS__
int main(void) {
    int a = 1;
    int b = 2;
    int c = LOG(a + b, 3);
    return c;
}
