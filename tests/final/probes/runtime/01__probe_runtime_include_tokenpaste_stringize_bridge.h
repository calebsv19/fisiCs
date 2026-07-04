#line 3910 "virtual_runtime_include_tokenpaste_stringize_bridge_phase01.h"
#define PHASE01_JOIN(a, b) a##b
#define PHASE01_NAME(tag) PHASE01_JOIN(tag, _mapped_line)
#define PHASE01_STR_INNER(x) #x
#define PHASE01_STR(x) PHASE01_STR_INNER(x)
enum { PHASE01_NAME(phase01_bridge) = __LINE__ };
static const char *phase01_bridge_text(void) { return PHASE01_STR(mapped + token); }
static int phase01_bridge_line_value(void) { return phase01_bridge_mapped_line; }
