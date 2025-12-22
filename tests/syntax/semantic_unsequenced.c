int f(int x, int y) { return x + y; }

int main(void) {
    int i = 0;
    i = i++ + i;          // should warn
    int a = f(i++, i);    // should warn
    int b = i;            // simple, no warning
    return a + b;
}
