// Expect: missing semicolon after a block-local declaration on line 4
int main(void) {
    {
        int x = 42
        return x;
    }
}
