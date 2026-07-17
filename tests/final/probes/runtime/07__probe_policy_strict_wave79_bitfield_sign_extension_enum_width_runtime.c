#include <stdio.h>

enum Width {
    WIDTH_NEGATIVE = -31,
    WIDTH_POSITIVE = 47
};

struct Fields {
    signed int narrow : 5;
    unsigned int selector : 1;
    unsigned int wide : 7;
};

int main(void) {
    struct Fields fields = {-9, 1u, 103u};
    int sign_extended = fields.narrow;
    enum Width selected = fields.selector ? WIDTH_NEGATIVE : WIDTH_POSITIVE;
    int combined = sign_extended + (int)selected;
    unsigned int widened = fields.wide + (unsigned int)(unsigned char)sign_extended;

    printf("%d %d %d %u\n", sign_extended, (int)selected, combined, widened);
    return 0;
}
