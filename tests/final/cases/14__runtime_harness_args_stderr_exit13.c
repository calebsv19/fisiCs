long write(int fd, const void* buf, unsigned long count);

int main(int argc, char** argv) {
    if (argc > 1 && argv[1] && argv[1][0] == 'k') {
        static const char ok[] = "ARGK";
        (void)write(2, ok, sizeof(ok) - 1);
        return 13;
    }
    static const char other[] = "ARGX";
    (void)write(2, other, sizeof(other) - 1);
    return 3;
}

