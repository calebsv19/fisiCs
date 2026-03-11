int printf(const char *fmt, ...);
char *getenv(const char *name);

int main(int argc, char **argv) {
    const char *v = getenv("FISICS_T");
    printf("%d %s %s\n", argc, v ? v : "null", (argc > 2) ? argv[2] : "none");
    return 0;
}
