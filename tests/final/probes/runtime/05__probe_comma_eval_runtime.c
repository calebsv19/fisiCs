int main(void) {
    int x = 0;
    int y = (x = 4, x + 3);
    return (x == 4 && y == 7) ? 0 : 1;
}
