#include <stdarg.h>

long wave17_stdarg_bridge_sum(const char *spec, ...);

int main(void) {
    long folded = wave17_stdarg_bridge_sum("idci", 4, 2.5, 'A', 7);
    return folded == 201 ? 0 : 1;
}
