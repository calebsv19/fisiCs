int main(void) {
    int x = 4;
    int y = 9;
    int ok = (x < y) && (x <= y) && (y > x) && (y >= x);
    return ok ? 0 : 1;
}
