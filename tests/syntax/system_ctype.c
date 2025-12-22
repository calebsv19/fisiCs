#include <ctype.h>

int main(void) {
    int ch = 'A';
    if (isalpha(ch)) {
        return tolower(ch);
    }
    return 0;
}
