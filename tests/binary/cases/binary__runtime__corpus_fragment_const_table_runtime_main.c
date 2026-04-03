unsigned table_checksum(void);
int printf(const char *fmt, ...);

int main(void) {
    printf("%u\n", table_checksum());
    return 0;
}
