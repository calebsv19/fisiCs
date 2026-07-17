extern int printf(const char *, ...);

#line 22200 "virtual_runtime_wave22_trigraph_phase01.c"
#define PHASE01_W22_TRI_STR_INNER(x) #x
#define PHASE01_W22_TRI_STR(x) PHASE01_W22_TRI_STR_INNER(x)
#define PHASE01_W22_TRI_TEXT "tri" ??/
/* phase-two trigraph splice joins this adjacent string sequence */ ??/
":splice-comment"

enum { phase01_wave22_trigraph_line = __LINE__ };

static unsigned phase01_wave22_checksum(const char *text) {
    unsigned value = 0;
    int index = 0;
    for (; text[index] != '\0'; ++index) {
        value = value * 33u + (unsigned char)text[index];
    }
    return value;
}

int main(void) {
    const char *text = "tri:" __FILE__ ":" PHASE01_W22_TRI_TEXT ":" PHASE01_W22_TRI_STR(phase01_wave22_trigraph_line);
    printf("%s|%d|%u\n", text, phase01_wave22_trigraph_line, phase01_wave22_checksum(text));
    return 0;
}
