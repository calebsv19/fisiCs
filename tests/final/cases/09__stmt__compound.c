int main(void) {
    int x = 1;
    {
        int x = 5;
        x = x + 1;
    }
    return x - 1;
}
