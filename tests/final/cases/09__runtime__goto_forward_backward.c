int main(void) {
    int x = 0;
    goto start;

loop:
    x += 3;
    if (x < 6) {
        goto loop;
    }
    return (x == 6) ? 0 : 1;

start:
    goto loop;
}
