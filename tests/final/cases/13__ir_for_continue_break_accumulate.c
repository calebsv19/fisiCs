int main(void) {
    int x = 0;
    int y = 0;

    for (int i = 0; i < 8; ++i) {
        if ((i & 1) == 0) {
            continue;
        }
        if (i > 5) {
            break;
        }
        y += i;
        x += y;
    }

    return x + y;
}
