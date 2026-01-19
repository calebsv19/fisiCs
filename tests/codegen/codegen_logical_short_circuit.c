int main(void) {
    int x = 0;
    int y = 0;
    int r = (x && (y = 4)) || (x || (y = 5));
    return y + r;
}
