extern int printf(const char *, ...);

#define PHASE01_W22_FILE "virtual_runtime_wave22_macro_filename_phase01.c"
#line 22300 PHASE01_W22_FILE
#define PHASE01_W22_STR_INNER(x) #x
#define PHASE01_W22_STR(x) PHASE01_W22_STR_INNER(x)
#define PHASE01_W22_RECORD0(file, line) "macro:" file ":" PHASE01_W22_STR(line)
#define PHASE01_W22_RECORD1(file, line) PHASE01_W22_RECORD0(file, line)
#define PHASE01_W22_RECORD2(file, line) PHASE01_W22_RECORD1(file, line)

enum { phase01_wave22_macro_line = __LINE__ };

static unsigned phase01_wave22_checksum(const char *text) {
    unsigned value = 0;
    int index = 0;
    for (; text[index] != '\0'; ++index) {
        value = value * 33u + (unsigned char)text[index];
    }
    return value;
}

int main(void) {
    const char *text = PHASE01_W22_RECORD2(__FILE__, phase01_wave22_macro_line);
    printf("%s|%d|%u\n", text, phase01_wave22_macro_line, phase01_wave22_checksum(text));
    return 0;
}
