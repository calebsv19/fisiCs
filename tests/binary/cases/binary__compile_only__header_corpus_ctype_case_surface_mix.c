#include <ctype.h>

int wave24_ctype_case_surface(const char *text) {
    int total = 0;
    while (*text) {
        unsigned char ch = (unsigned char)*text++;
        total += tolower(ch);
        total -= toupper(ch);
    }
    return total;
}
