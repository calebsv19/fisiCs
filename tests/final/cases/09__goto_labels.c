int main(void) {
    int x = 0;
start:
    x++;
    if (x < 2) goto start;
    return x;
}
