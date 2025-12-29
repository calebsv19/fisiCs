#include <stddef.h>

struct P {
    int a;
    unsigned b;
};

struct Bit {
    unsigned a : 3;
    unsigned b : 5;
};

const char msg[] = "hi";
char padded[5] = "abc";
const char *ptr = "xyz";
int ints[4] = {1, 2};
struct P gp = { .a = 1, .b = 2 };
static struct P gp2 = (struct P){3, 4};
struct Bit gb = { .a = 5, .b = 7 };

int main(void) {
    return gp.a + gp2.b + (int)padded[4] + (int)gb.a + (int)ints[1] + (ptr != NULL);
}
