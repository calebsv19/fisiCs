int main(void) {
    int x = 2;

    switch (x) {
        default:
            x += 1;
            break;
        default:
            x += 2;
            break;
    }

    return x;
}
