#if __COUNTER__ != 0
#error "__COUNTER__ should start at 0"
#endif

#if __COUNTER__ != 1
#error "__COUNTER__ second use should be 1"
#endif

#define ADD_IF_ARGS(base, ...) base __VA_OPT__(+ (__VA_ARGS__))

#line 300 "virt.c"
int line_value = __LINE__;
const char* file_name = __FILE__;
const char* base_file = __BASE_FILE__;

int vaopt_with = ADD_IF_ARGS(10, 2);
int vaopt_without = ADD_IF_ARGS(10);

int counter_third = __COUNTER__;
int counter_fourth = __COUNTER__;
