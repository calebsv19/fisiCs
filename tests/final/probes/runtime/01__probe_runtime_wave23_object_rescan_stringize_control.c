#include <stdio.h>

#define PHASE01_W23_CAT_RAW(a, b) a##b
#define PHASE01_W23_CAT(a, b) PHASE01_W23_CAT_RAW(a, b)
#define PHASE01_W23_STR_RAW(x) #x
#define PHASE01_W23_STR(x) PHASE01_W23_STR_RAW(x)

#define PHASE01_W23_ROOT alpha
#define alpha_value 73
#define PHASE01_W23_VALUE(name) PHASE01_W23_CAT(name, _value)

#line 23100 "virtual_wave23_object_rescan_control_phase01.c"
enum { phase01_wave23_object_value = PHASE01_W23_VALUE(PHASE01_W23_ROOT) };

int main(void) {
    const char *expanded = PHASE01_W23_STR(PHASE01_W23_VALUE(PHASE01_W23_ROOT));
    const char *raw = PHASE01_W23_STR_RAW(PHASE01_W23_VALUE(PHASE01_W23_ROOT));
    printf("%d %d %d %s %d\n",
           phase01_wave23_object_value,
           expanded[0] == '7' && expanded[1] == '3' && expanded[2] == '\0',
           raw[0] == 'P',
           __FILE__,
           __LINE__);
    return 0;
}
