int sum(int count, ...);
int sum(int count, ...) { return count; }
int main(void) {
    int a = sum(2, 1, 2);
    int b = sum(3, 1, 2, 3);
    return a + b;
}
