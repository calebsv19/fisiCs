extern int printf(const char *, ...);

#line 22100 "virtual_runtime_wave22_layered_phase01.c"
#define PHASE01_W22_STR_INNER(x) #x
#define PHASE01_W22_STR(x) PHASE01_W22_STR_INNER(x)
#define PHASE01_W22_STAGE0(x) x
#define PHASE01_W22_STAGE1(x) PHASE01_W22_STAGE0(x)
#define PHASE01_W22_STAGE2(x) PHASE01_W22_STAGE1(x)

enum { phase01_wave22_layered_line = __LINE__ };

static unsigned phase01_wave22_checksum(const char *text) {
    unsigned value = 0;
    int index = 0;
    for (; text[index] != '\0'; ++index) {
        value = value * 33u + (unsigned char)text[index];
    }
    return value;
}

int main(void) {
    const char *text = PHASE01_W22_STAGE2("layered:" __FILE__ ":" PHASE01_W22_STR(phase01_wave22_layered_line));
    printf("%s|%d|%u\n", text, phase01_wave22_layered_line, phase01_wave22_checksum(text));
    return 0;
}
