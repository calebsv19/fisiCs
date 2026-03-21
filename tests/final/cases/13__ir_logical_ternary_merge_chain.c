int main(void) {
    int x = 1;
    int y = 2;
    int z = 0;
    z = (x < y) ? (x + y) : (x - y);
    z += (x && y) ? 3 : 4;
    return z;
}
