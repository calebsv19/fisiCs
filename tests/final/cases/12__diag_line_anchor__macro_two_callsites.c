#define BAD (missing() + 1)

int main(void) {
    int a = BAD;
    int b = BAD;
    return a + b;
}
