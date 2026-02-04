typedef int T;
int main(void) {
    int x = 0;
    int a = sizeof(T);
    int b = sizeof x;
    return a + b;
}
