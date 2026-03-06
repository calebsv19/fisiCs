#define SUM(base, ...) (base __VA_OPT__(+ __VA_ARGS__))

int a = SUM(1);
int b = SUM(1, 2);
