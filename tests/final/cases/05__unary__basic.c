int main(void) {
    int x = 3;
    int y = -x;
    int z = +x;
    int ok = !0;
    return (y == -3 && z == 3 && ok == 1) ? 0 : 1;
}
