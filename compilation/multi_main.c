// Minimal write prototype to avoid depending on system headers.
long write(int fd, const void* buf, unsigned long count);

int helper(void);

int main(void) {
    int h = helper();
    int total = h + 5;
    const char* msg = "multi-file smoke: helper=7 total=12\n";
    write(1, msg, 35); // length without trailing null
    return (h == 7 && total == 12) ? 0 : 1;
}
