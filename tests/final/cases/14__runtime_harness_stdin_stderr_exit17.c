long read(int fd, void* buf, unsigned long count);
long write(int fd, const void* buf, unsigned long count);

int main(void) {
    char ch = 0;
    long n = read(0, &ch, 1);
    if (n == 1 && ch == 'q') {
        static const char qpath[] = "QPATH";
        (void)write(2, qpath, sizeof(qpath) - 1);
        return 17;
    }
    static const char npath[] = "NPATH";
    (void)write(2, npath, sizeof(npath) - 1);
    return 5;
}

