// Minimal write prototype to avoid depending on system headers.
long write(int fd, const void* buf, unsigned long count);

int main(void) {
    int sum = 2 + 3;
    const char* msg = "single-file smoke: add 2 + 3 = 5\n";
    write(1, msg, 34); // known length without trailing null
    return (sum == 5) ? 0 : 1;
}
