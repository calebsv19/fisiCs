// Expect: missing semicolon after continue on line 5
int main(void) {
    int sum = 0;
    for (int i = 0; i < 3; ++i) {
        continue
        sum += i;
    }
    return sum;
}
