// Expect: missing semicolon after break on line 6
int main(void) {
    for (int i = 0; i < 3; ++i) {
        if (i == 1) {
            break
        }
    }
    return 0;
}
