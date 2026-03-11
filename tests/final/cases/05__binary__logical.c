int main(void) {
    int a = 1;
    int b = 0;
    int c = (a && b) || (a && !b);
    return c ? 0 : 1;
}
