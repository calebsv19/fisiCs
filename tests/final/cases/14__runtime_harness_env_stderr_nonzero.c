long read(int fd, void* buf, unsigned long count);
long write(int fd, const void* buf, unsigned long count);

int main(void) {
    char ch = 0;
    long n = read(0, &ch, 1);
    if (n == 1 && ch == 'x') {
        static const char ok[] = "stdin=x\n";
        (void)write(2, ok, sizeof(ok) - 1);
        return 9;
    }
    static const char other[] = "stdin=other\n";
    (void)write(2, other, sizeof(other) - 1);
    return 4;
}
