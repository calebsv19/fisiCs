int main(void) {
    int a = 0;
    int b = 1;
    int c = 2;
    int out = (a ? (b + 3) : (c + 5)) + ((b && c) ? 7 : 9);
    return out;
}
