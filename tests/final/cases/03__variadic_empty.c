#define LOG(fmt, ...) fmt __VA_ARGS__
int main(void) {
    int x = LOG(1);
    return x;
}
