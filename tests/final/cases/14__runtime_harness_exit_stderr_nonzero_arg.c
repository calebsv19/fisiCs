long write(int fd, const void* buf, unsigned long count);

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    static const char msg[] = "exit=37\n";
    (void)write(2, msg, sizeof(msg) - 1);
    return 37;
}
