short s = -1;
unsigned short us = 2;
int i = 3;
unsigned int ui = 4u;
long l = 5L;
unsigned long ul = 6UL;
long long ll = 7LL;
unsigned long long ull = 8ULL;

int sum_primitives(void) {
    return (int)s + (int)us + i + (int)ui + (int)l + (int)ul + (int)ll + (int)ull;
}
