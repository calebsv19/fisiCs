int main(void) {
    int x = 0;
    int y = sizeof x + 1;
    int z = sizeof(int) + 2;
    return y + z;
}
