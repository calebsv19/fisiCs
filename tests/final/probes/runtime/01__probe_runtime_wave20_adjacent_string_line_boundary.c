extern int printf(const char*, ...);

#line 9810 "virtual_runtime_wave20_adjacent_boundary_phase01.c"
#define PHASE01_W20_ADJ_STR_INNER(x) #x
#define PHASE01_W20_ADJ_STR(x) PHASE01_W20_ADJ_STR_INNER(x)
enum { phase01_wave20_adjacent_before_line = __LINE__ };
static const char *phase01_wave20_adjacent_text(void) {
    return "adj:" __FILE__ ":" "alpha"
"-" "beta"
":" PHASE01_W20_ADJ_STR(phase01_wave20_adjacent_before_line);
}
enum { phase01_wave20_adjacent_after_line = __LINE__ };

int main(void) {
    printf("%s|%d|%d|%s\n",
           phase01_wave20_adjacent_text(),
           phase01_wave20_adjacent_before_line,
           phase01_wave20_adjacent_after_line,
           __FILE__);
    return 0;
}
