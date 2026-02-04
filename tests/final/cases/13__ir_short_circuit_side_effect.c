int main(void) {
    int x = 0;
    int y = x && (x = 1);
    return x + y;
}
