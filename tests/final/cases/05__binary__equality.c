int main(void) {
    int x = 12;
    int y = 12;
    int z = 5;
    int ok = (x == y) && (x != z);
    return ok ? 0 : 1;
}
